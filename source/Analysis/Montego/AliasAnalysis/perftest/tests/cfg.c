void empty()
{
}

void multiBlocks()
{
    {
        {
        }
        {
        }
    }
}

void linear()
{
    int x, y, z;
    x = 0;
    y = 10;
    z = x+y;
}

void ifStmt()
{
    int x,y;
    if (x > 0)
        x = 1;
    else
        x = -1;
    y = x;
}

void switchStmt()
{
    int x, y, z;
    switch(x)
    {
    case 1:
    case 2:
        x = y;
        break;
    case 3:
        x = z;
    default:
        z = y;
    }
}

void forStmt()
{
    int i, x;
    for(i=0; i< 10; ++i)
        x *=2;
}

void forStmtGoto()
{
    int i, x;
    goto l;
    for(i=0; i < 10; ++i)
    {
        l: x++;
    }
}

void whileStmt()
{
    while(0)
    {
    }
    1+1;
}

void doWhileStmt()
{
    do
    {

    }
    while(0);
    1+1;
}
