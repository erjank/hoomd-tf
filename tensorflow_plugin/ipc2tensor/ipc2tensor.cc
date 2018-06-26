#include "ipc2tensor.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/platform/logging.h"
#include <sys/mman.h>
#include <typeinfo>

using namespace tensorflow;


using CPUDevice = Eigen::ThreadPoolDevice;
using GPUDevice = Eigen::GpuDevice;

// CPU specialization of actual computation.
template <typename T>
struct IPC2TFunctor<CPUDevice, T> {
  void operator()(const CPUDevice& d, int size, int64 address, T* out) {
    //TODO: access address
    Scalar4* input_buffer = reinterpret_cast<Scalar4*> (address);
    for(int i = 0; i < size; ++i) {
      out[4 * i + 0] = input_buffer[i].x;
      out[4 * i + 1] = input_buffer[i].y;
      out[4 * i + 2] = input_buffer[i].z;
      out[4 * i + 3] = input_buffer[i].w;
    }
  }
};

// CPU Initializer
template<>
struct IPC2TInitialize<CPUDevice> {
  bool operator()(int size, int64 address) {
    // check shared memory
    Scalar4* input_buffer = reinterpret_cast<Scalar4*> (address);
    LOG(INFO) << "about to try reading from " << std::hex << address << " with type " << typeid(input_buffer).name() << std::endl;
    return true;
  }
};


// OpKernel definition.
// template parameter <T> is the datatype of the tensors.
template <typename Device, typename T>
class IpcToTensorOp : public OpKernel {
 public:
  explicit IpcToTensorOp(OpKernelConstruction* c) : OpKernel(c) {

    LOG(INFO) << "IpcToTensorOp construction starting";
    //get number of atoms
    c->GetAttr("size", &_input_size);

    //get memory address
    c->GetAttr("address", &_input_address);

    int temp_dims [2] = {_input_size, 4};
    //TODO: why is this necessary?!
    TensorShapeUtils::MakeShape(temp_dims, 1, &_output_shape);

    //call device initializer
    OP_REQUIRES(c, IPC2TInitialize<Device>()(_input_size,
                              _input_address),
                errors::FailedPrecondition("Memory mapped buffer not accessible or invalid."));
    LOG(INFO) << "OP constructed and mmap connection validated";

  }

  void Compute(OpKernelContext* context) override {

    // Create an output tensor
    Tensor* output_tensor = NULL;
    OP_REQUIRES_OK(context, context->allocate_output(0, _output_shape,
                                                     &output_tensor));

    // Do the computation.
    OP_REQUIRES(context, output_tensor->NumElements() <= tensorflow::kint32max,
                errors::InvalidArgument("Too many elements in tensor"));
    IPC2TFunctor<Device, T>()(
        context->eigen_device<Device>(),
        _input_size,
        _input_address,
        output_tensor->flat<T>().data());
  }

private:
  int _input_size;
  int64 _input_address;
  TensorShape _output_shape;
};

// Register the CPU kernels.
#define REGISTER_CPU(T)                                          \
  REGISTER_KERNEL_BUILDER(                                       \
      Name("IpcToTensor").Device(DEVICE_CPU).TypeConstraint<T>("T"), \
      IpcToTensorOp<CPUDevice, T>);
REGISTER_CPU(float);


// Register the GPU kernels.
#ifdef GOOGLE_CUDA
#define REGISTER_GPU(T)                                          \
  /* Declare explicit instantiations in kernel_IPC2T.cu.cc. */ \
  extern template IPC2TFunctor<GPUDevice, float>;              \
  REGISTER_KERNEL_BUILDER(                                       \
      Name("IpcToTensor")
      .Device(DEVICE_GPU).TypeConstraint<T>("T")
      .HostMemory("shape")
      .HostMemory("address"), \
      IpcToTensorOp<GPUDevice, T>);
REGISTER_GPU(float);
#endif  // GOOGLE_CUDA
