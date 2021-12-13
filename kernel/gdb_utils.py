class Vir2PhysAddrImpl(Vir2PhysAddr):
    """$p(arg): Convert virtual address to physical address."""

    def __init__(self):
        super(Vir2PhysAddrImpl, self).__init__(0x800)

Vir2PhysAddrImpl()
