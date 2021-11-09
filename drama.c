#include <stdio.h>
#include <string.h>
#include <ctype.h>

int my_strcmp(char *s1, char *s2);

int main()
{
    char s1[100], s2[100];
    int SZEREPLO = 0;

    char drama[30], szereplo[20], buffer[30];
    scanf("%s %s", drama, szereplo);

    snprintf(buffer, sizeof(char) * 30, "%s.txt", szereplo);

    FILE* out;
    out = fopen(buffer, "wt");

    for(int i = 0; szereplo[i] != '\0'; i++)
    {
        szereplo[i] = toupper(szereplo[i]);
    }

    FILE* ptr;
    ptr = fopen(("%s", drama), "rt");
    if(ptr == NULL)
    {
        return 1;
    }
    
    while(fgets(s1, 100, ptr) != NULL)
    {
        if(SZEREPLO == 1 && s1[0] == '\t')
        {
            for(int j = 0; s1[j] != '\0'; j++)
            {
                if(s1[j] != '\t')
                    fprintf(out, "%c", s1[j]);
            }
        }
        else if(s1[0] == '\n' && SZEREPLO == 1)
            fprintf(out, "\n");
        else if(my_strcmp(s1, szereplo) != 0 && s1[0] != '\t')
        {
            SZEREPLO = 0;
        }
            
        
        if(my_strcmp(s1, szereplo) == 0)
        {
            SZEREPLO = 1;
        }
    }

    fclose(ptr);
    fclose(out);
    return 0;

}

int my_strcmp(char *s1, char *s2)
{
    for(int i = 0; s2[i] != '\0'; i++)
    {
        if(s1[i] != s2[i])
            return 1;
    }
    return 0;
}