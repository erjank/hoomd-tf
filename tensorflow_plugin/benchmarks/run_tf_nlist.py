import hoomd, hoomd.md, math
import hoomd.tensorflow_plugin

save_loc = '/tmp/benchmark-gradient-potential-model'
N = 1024
rcut = 5.0

hoomd.context.initialize()
system = hoomd.init.create_lattice(unitcell=hoomd.lattice.sq(a=4.0),
                                    n=int(math.ceil(N**(1/2))))
nlist = hoomd.md.nlist.cell()
hoomd.md.integrate.mode_standard(dt=0.005)
#lj = hoomd.md.pair.lj(r_cut=3.0, nlist=nlist)
#lj.pair_coeff.set('A', 'A', epsilon=1.0, sigma=1.0)
hoomd.md.integrate.nve(group=hoomd.group.all())
tfcompute = hoomd.tensorflow_plugin.tensorflow(save_loc, nlist, r_cut=rcut, debug_mode=False)
hoomd.run(1000, profile=True)

hoomd.context.initialize()
system = hoomd.init.create_lattice(unitcell=hoomd.lattice.sq(a=4.0),
                                    n=int(math.ceil(N**(1/2))))
nlist = hoomd.md.nlist.cell()
#lj = hoomd.md.pair.lj(r_cut=3.0, nlist=nlist)
#lj.pair_coeff.set('A', 'A', epsilon=1.0, sigma=1.0)
hoomd.md.integrate.mode_standard(dt=0.005)
hoomd.md.integrate.nve(group=hoomd.group.all())
hoomd.run(1000, profile=True)