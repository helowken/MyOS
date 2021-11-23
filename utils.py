class Vir2PhysAddr:
    def __init__(self, offset):
        self.__offset = offset

    def convert(self, v):
        vt = v.type
        pre_vt = None
        if v.address == None:
            if vt.code == gdb.TYPE_CODE_PTR:
                addr = int(v.dereference().address)
                pre_vt = vt
                vt = vt.target()
            else:
                raise Exception("No address, may be in register.")
        else:
            addr = int(v.address)

        i = 0
        while vt.code == gdb.TYPE_CODE_PTR:
            addr += self.__offset
            print('%s, %s: %s' % (i, vt, hex(addr)))
            i += 1
            s = '*(char **)%s' % hex(addr)
            addr = gdb.parse_and_eval(s)
            if addr == 0:
                return 0
            pre_vt = vt
            vt = vt.target()

        addr += self.__offset
        print('%s, %s: %s' % (i, vt, hex(addr)))
        ptr = '*'
        if pre_vt != None and str(pre_vt) == 'char *':
            ptr = ''
        s = '%s(%s *)%s' % (ptr, vt, hex(addr))
        print(s)
        return gdb.parse_and_eval(s)


class ProtectedVir2PhysAddr(gdb.Function):
    """$p(arg): Convert protected mode virtual address to physical address."""

    def __init__(self):
        super(ProtectedVir2PhysAddr, self).__init__('p')
        self.__converter = Vir2PhysAddr(0x1000)

    def invoke(self, *args):
        return self.__converter.convert(args[0])


class RealVir2PhysAddr(gdb.Function):
    """$r(arg): Convert real mode virtual address to physical address."""

    def __init__(self):
        super(RealVir2PhysAddr, self).__init__('r')
        self.__converter = Vir2PhysAddr(0x90000)

    def invoke(self, *args):
        return self.__converter.convert(args[0])


ProtectedVir2PhysAddr()
RealVir2PhysAddr()
