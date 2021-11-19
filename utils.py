class ConvertKernelPhysAddr(gdb.Function):
    """$k(arg): Convert address to kernel physical address."""

    def __init__(self):
        super(ConvertKernelPhysAddr, self).__init__("kpa")

    def _tostr(self, v):
        try:
            return v.string()
        except gdb.error:
            return str(v)
    
    def invoke(self, *args):
        v = args[0]
        els = ['*(', str(v.type)]
        if v.type.code == gdb.TYPE_CODE_PTR:
            addr = int(v.cast(gdb.lookup_type("long")))
        else:
            addr = int(v.address)
            els.append(' *')
        addr += 0x1000;
        els.append(')')
        els.append(hex(addr))
        s = ''.join(els)
        return gdb.parse_and_eval(s)

ConvertKernelPhysAddr()
