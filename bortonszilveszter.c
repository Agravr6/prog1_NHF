int jail(int t[])
{
    int back = 9;
    int front = 0;
    int wrongly_placed = 0;

    while(front < back)
    {
        while(t[back] == 0)
            back--;
        if(front == back)
            break;
        while(t[front] == 10)
            front++;
        if(front > back)
            break;
        
        t[back]--;
        t[front]++;
        if((back % 2) != (front % 2))
            wrongly_placed++;
    }
    return wrongly_placed;
}
