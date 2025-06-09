struct BAKERY
{
    int num;
    char res;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        struct BAKERY GET_NUM(struct BAKERY) = 1;
        struct BAKERY GET_RES(struct BAKERY) = 2;
    } = 1;
} = 0x2000001;
