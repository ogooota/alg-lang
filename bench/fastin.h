struct FastIn
{
    static char buf[1 << 16];
    size_t len = 0, pos = 0;
    int get()
    {
        if (pos == len)
        {
            len = fread(buf, 1, sizeof buf, stdin);
            pos = 0;
            if (!len)
                return -1;
        }
        return buf[pos++];
    }
    template <class T> FastIn &operator>>(T &x)
    {
        int c = get();
        while (c != '-' && (c < '0' || c > '9'))
            c = get();
        bool neg = c == '-';
        if (neg)
            c = get();
        x = 0;
        while (c >= '0' && c <= '9')
        {
            x = x * 10 + (c - '0');
            c = get();
        }
        if (neg)
            x = -x;
        return *this;
    }
} fin;
char FastIn::buf[1 << 16];
