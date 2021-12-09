class Vir2PhysAddr(gdb.Function):
    def __init__(self, offset):
        super(Vir2PhysAddr, self).__init__('p')
        self.__offset = offset

    def invoke(self, *args):
        v = args[0]
        vt = v.type
        pre_vt = None
        if v.address == None:
            if vt.code == gdb.TYPE_CODE_PTR:
                # For 'gdt', it is a pointer but has no address.
                addr = int(v.dereference().address)
                pre_vt = vt
                vt = vt.target()
            else:
                # For 'register int i', there is no address.
                raise Exception("No address, may be in register.")
        else:
            addr = int(v.address)

        i = 0
        # Dereference pointer and add offset to its address.
        while vt.code == gdb.TYPE_CODE_PTR:
            addr += self.__offset
            print('%s, %s: %s' % (i, vt, hex(addr)))
            i += 1
            s = '*(char **)%s' % hex(addr)
            addr = gdb.parse_and_eval(s)
            # The pointer's value is NULL.
            if addr == 0:
                return 0
            pre_vt = vt
            vt = vt.target()

        addr += self.__offset
        print('%s, %s: %s' % (i, vt, hex(addr)))

        # For (char *), its target type is int, so we use str() to do the check.
        ptr = '*'
        if pre_vt != None and str(pre_vt) == 'char *':
            ptr = ''

        s = '%s(%s *)%s' % (ptr, vt, hex(addr))
        print(s)
        return gdb.parse_and_eval(s)

