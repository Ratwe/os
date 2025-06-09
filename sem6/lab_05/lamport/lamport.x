struct BAKERY
{
    int id;
    int num;
    char symb;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        struct BAKERY GET_NUM(struct BAKERY) = 1;
        struct BAKERY GET_RES(struct BAKERY) = 2;
	struct BAKERY WAIT_RES(struct BAKERY) = 3;
    } = 1;
} = 0x2000001;
