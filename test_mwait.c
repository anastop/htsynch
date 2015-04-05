static inline void monitor(const void *eax, unsigned long ecx,
                        unsigned long edx)
{
    /* "monitor %eax,%ecx,%edx;" */
    asm volatile(
            ".byte 0x0f,0x01,0xc8;"
            : 
            :"a" (eax), "c" (ecx), "d"(edx));
}

static inline void mwait(unsigned long eax, unsigned long ecx)
{
    /* "mwait %eax,%ecx;" */
    asm volatile(
            ".byte 0x0f,0x01,0xc9;"
            : 
            :"a" (eax), "c" (ecx));
}

char c;
int main(int argc, char **argv)
{
    monitor(&c, 0, 0);
    mwait(0,0); 
}
