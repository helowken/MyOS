import os

class Vir2PhysAddr(gdb.Function):
    def __init__(self, func_name, offset):
        super(Vir2PhysAddr, self).__init__(func_name)
        self.offset = int(offset, base=16)

    def invoke(self, *args):
        size = len(args)
        if size == 0:
            raise Exception("No variable!")
        (addr, vt, pre_vt) = self._calc_addr(args[0])
        return self.__parse(addr, vt, pre_vt)

    def _calc_addr(self, v):
        pass

    def _get_addr(self, v):
        addr = int(v.address)
        print(' addr: %s' % hex(addr))
        return addr

    def _get_pointer_addr(self, v):
        vt = v.type
        if vt.code == gdb.TYPE_CODE_PTR:
            addr = int(v.dereference().address)
            print('deref addr: %s' % hex(addr))
            pre_vt = vt
            vt = vt.target()
            return (addr, vt, pre_vt)
        else:
            raise Exception("Deref addr failed, may be in register.")

    def _relocate(self, vt, addr):
        old_addr = addr
        addr += self.offset
        print('reloc: (%s) %s => %s' % (vt, hex(old_addr), hex(addr)))
        return addr

    def _deref_addr(self, addr):
        s = '*(char **) %s' % hex(addr)
        nv = gdb.parse_and_eval(s)
        addr = int(nv)
        print('deref: %s => %s' % (s, hex(addr)))
        return addr

    def __parse(self, addr, vt, pre_vt):
        # Dereference pointer and add offset to its address.
        while vt.code == gdb.TYPE_CODE_PTR:
            addr = self._relocate(vt, addr)
            self._deref_addr(addr)
            # The pointer's value is NULL.
            if addr == 0:
                return 0
            pre_vt = vt
            vt = vt.target()

        addr = self._relocate(vt, addr)

        # For (char *), its target type is int, so we use str() to do the check.
        ptr = '*'
        if pre_vt != None and str(pre_vt).replace('const', '').strip() == 'char *':
            ptr = ''

        s = '%s(%s *) %s' % (ptr, vt, hex(addr))
        print('parse: %s' % s)
        print('   ------------------')
        return gdb.parse_and_eval(s)


class DefaultPrinter(Vir2PhysAddr):
    """$p(arg): Convert virtual address to physical address."""

    def __init__(self, offset):
        super(DefaultPrinter, self).__init__('p', offset)

    def _calc_addr(self, v):
        if v.address == None:
            return self._get_pointer_addr(v)
        addr = self._get_addr(v)
        return (addr, v.type, None)


class FieldPointerPrinter(Vir2PhysAddr):
    """$r(arg): Convert virtual address to physical address, for field pointer."""

    def __init__(self, offset):
        super(FieldPointerPrinter, self).__init__('r', offset)

    def _calc_addr(self, v):
        return self._get_pointer_addr(v)


class StackPointerPrinter(Vir2PhysAddr):
    """$s(arg): Convert virtual address to physical address, for pointer on stack."""

    def __init__(self, offset):
        super(StackPointerPrinter, self).__init__('s', offset)

    def _calc_addr(self, v):
        vt = v.type
        if vt.code == gdb.TYPE_CODE_PTR:
            addr = self._get_addr(v)
            addr = self._relocate(vt, addr)
            addr = self._deref_addr(addr)
            return (addr, vt.target(), None)
        raise Exception("Not a pointer.")



# get offset from env
my_module = os.getenv('MY_MODULE')
if my_module is None:
    raise Exception("No module found in env.")
offset = os.getenv(my_module)
if offset is None:
    raise Exception("No offset bound for module '%s'." % my_module)
print('=== Debug module: %s, offset: %s ===' % (my_module, offset))

# register functions
DefaultPrinter(offset)
FieldPointerPrinter(offset)
StackPointerPrinter(offset)








