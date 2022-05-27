/* Varga Csaba - K0**** - 2021 - A programozás alapjai 1 - Nagy házi feladat
A program a paraméterlistán megkapott fájlokból beolvassa az adatokat és eltárolja egy fésűs listában. Ezután kiírja a standard outputra az ügyfelek 
számát és az eloszlásukat a biztosítási csomagok között. Végül optimalizálja a biztosítási csomagok árait és  kiírja a 
standard outputra, telefononként csoportosítva, az ügyfelek nevét (abc sorrendben) és az ő általuk választott telefon és biztosítási csomag árát (összeadva).*/

/*
TODO
    testing
    documentaion
*/
/*
Test cases: -üres - TESTED
            -negatív értékek - TESTED
            -azonos nevek - TESTED
            -input adatok pontos mérete és típusa - TESTED
            -hibás fájlnév parancssori argumentumként - TESTED
            -kevés/sok argumentum - TESTED/TESTED
            -nagyon sok adat
            -mindenki 1 telefont választ - TESTED
            -van telefon amit nem választ senki - TESTED
            -ugyanaz az ugyfel/device azonosito - TESTED
            -invalid biztositasi csomag - TESTED
            -biztosítási csomagok árcsökkentése ki van maxolva (nem csökken-e túlzottan)
            -parancssori argumentumok felcserélve  - TESTED
            -fájlokban pontosan 2x 3x stb annyi adat van 1 sorban
            -fájlokban nem mindenhol 2 space választja el az adatokat (többnyire a számokat kell figyelni, hogy ne legyen benne space)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "debugmalloc.h" // memory leak detection

// Az egyes biztosítási csomagok alapárának a meghatározása
// nem szép megoldás, majdnem meghúztak miatta
// enum-ot lehet helyette használni
const double CLASS_1 = 15.0;
const double CLASS_2 = 20.0;
const double CLASS_3 = 40.0;

// készülékek tárolására használt struktúra
typedef struct {
    long device_ID; // készülékazonosító PL: 123456789
    char name[100]; // készülék neve PL: Iphone 8
    long price; // készülék ára PL: 900
} device;

// ügyfeleket tároló struktúra
typedef struct {
    char name[100]; // ügyfél neve PL: Fekete Bence
    int customer_ID, insurance_type; // ügyfélazonosító PL: 123456, biztosítási csomag PL: 2
} customer;

// fésűs lista oldalága
typedef struct _sidenode {
    customer data; // a listaelem adati
    struct _sidenode *next; // következő elemre mutató pointer
} side_node;

// fésűs lista gerince
typedef struct _node {
    device data; // a listaelem adatai
    struct _node *next; // következő elemre mutató pointer
    struct _sidenode *under; // az adott gerincelemhez csatlakozó oldalág első elemére mutató pointer
} main_node;

// globális változók. A különböző biztosítási csomagok árát és az adott csomagot választott emberek mennyiségét tárolják
double class1_price, class2_price, class3_price;
int class1_count = 0, class2_count = 0, class3_count = 0;

main_node *read_device();
long valid_price();
int read_customer();
int customer_count();
int compare_strings();
double calculate_final_price();
void price_optimalization();
void print_result();
void free_list();
int check_for_existing_cusomer_ID();
int check_for_existing_device_ID();
int check_for_same_name();
int valid_dev_id_len();
int valid_customer_id_len();
int argc_each_line_in_file();

// Merge sort source: https://www.geeksforgeeks.org/merge-sort-for-linked-list/
side_node* SortedMerge(side_node* a, side_node* b);
void FrontBackSplit(side_node* source, side_node** frontRef, side_node** backRef);
void MergeSort(side_node** headRef);

// parancssori argumentumok: argc: az argumentumok száma argv[]: az argumentumokat tartalmazza stringként
int main(int argc, char *argv[])
{
    // ellenőrzi, hogy megfelelő mennyiségű paracssori argumentum érkezett
    if (argc != 3)
    {
        printf("Useage: <file name with device data> <file name with customer data>"); // használati segítség
        return 1;
    }

    FILE *fpdevice, *fpcustomer;
    // Az adatokat tartalmazó txt fájlok megnyitása
    fpdevice = fopen(argv[1], "rt");
    if (fpdevice == NULL) // Ha hiba történik a fájl megnyitása közben
    {
        printf("Device file opening error\n");
        return 1;
    }
    fpcustomer = fopen(argv[2], "rt");
    if (fpcustomer == NULL) // Ha hiba történik a fájl megnyitása közben
    {
        printf("Customer file opening error\n");
        return 1;
    }

    main_node *list = read_device(fpdevice);
    if(list == NULL) // ha nincsenek listaelemek akkor a program leállítja magát
        return 1;

    int return_customer = read_customer(list, fpcustomer);  // a visszatérési érték 1, ha üres a fájl
    if(return_customer == 1)
    {
        free_list(list);
        return 1;
    }
    else if(return_customer == -1)  // -1 visszatérési érték: érvénytelen a biztosítási csomag típusa
    {
        printf("Invalid insurance type\n");
        free_list(list);
        return 1;
    }
    else if(return_customer == -2 || return_customer == -3 || return_customer == -4)
    {
        free_list(list);
        return 1;
    }

    int customer_cnt = customer_count(list, &class1_count, &class2_count, &class3_count); // customer_cnt eltárolja az ügyfelek számát
    price_optimalization(&class1_price, &class2_price, &class3_price);

    main_node *p = list;
    while(p != NULL) // végigjárva a fésűs lista gerincét minden oldalágra elvégzi a nevek sorba rendezését (abc szerint)
    {
        side_node *p_side = p->under;
        MergeSort(&p_side);
        p->under = p_side;
        p = p->next;
    }

    print_result(list, customer_cnt); // kiírja az eredményeket a standard outputra
    free_list(list); // listaelemek felszabadítása

    return 0;
}

/*Paraméterként megkap egy fájlra mutató pointert, a fájlban lévő adatokat beolvassa
 és eltárolja őket egy fésűs láncolt lista gerincén. Visszatérési értéke a láncolt listára mutató pointer*/
main_node *read_device(FILE *fp)
{
    char buffer[200];
    int is_empty = 1;
    main_node *head = NULL;
    while(fgets(buffer, 200, fp) != NULL)  // soronként beolvassa az adatokat az input fájlból
    {
        if(argc_each_line_in_file(buffer) != 3) // ellenőrzi, hogy megfelelő mennyiségű adat van-e az adott sorban
        {
            // ha nincs, akkor az addig lefoglalt memóriát felszabadítja és hibával tér vissza
            free_list(head);
            printf("Too many or not enough data in a line of the device file\n");
            return NULL;
        }
        is_empty = 0;
        long dev_ID;
        sscanf(buffer, "%li", &dev_ID);
        if(check_for_existing_device_ID(dev_ID, head) == 1) // Ellenőrzi, hogy van-e a már elmentett adatok között olyan készülékazonosító mint amilyet most akarunk elmenteni
        {  
            // ha van, akkor az addig lefoglalt memóriát felszabadítjuk és hibával térünk vissza
            printf("Already existing device ID\n");
            free_list(head);
            return NULL;
        }
        if(valid_dev_id_len(dev_ID) == 0) // ellenőrzi, hogy megfelelő hosszúságú-e az azonosító
        {
            printf("Invalid device id\n");
            free_list(head);
            return NULL;
        }
        main_node *p = (main_node*)malloc(sizeof(main_node)); // memóriát foglal egy új gerincelemnek
        p->data.device_ID = dev_ID;


        // feldolgozza a készülék nevét stringként
        int len_of_name = 0, i = 11; // 11: azért mert a készülék azonosító 9 karakter hosszú és utána van még 2 space, tehát a telefon neve a 11. indexen kezdődik
        while(1)
        {
            if(buffer[i] == ' ')
                if(buffer[i + 1] == ' ')
                    break;
            len_of_name++;
            i++;
        }
        char device_name[100];

        memcpy(device_name, buffer + 11, len_of_name); // A telefon nevét átmásolja egy ideiglenes változóba
        device_name[len_of_name] = '\0';  // a string végére odateszi a végjelet
        strcpy(p->data.name, device_name); // a módosított stringet elmenti az újonnan foglalt listaelembe

        int len_of_price = 0, j = len_of_name + 11; // j értéke: 11 karakter van a telefon nevéig utána pedig hozzá kell adni még a telefon nevének a hosszát és ott fog kezdődni az ár
        while(1)
        {
            if(buffer[j] == '\n')
                break;
            len_of_price++;
            j++;
        }
        char *device_price = (char*)malloc(sizeof(char) * len_of_price); // memóriát foglalunk az pénzösszegnek
        sscanf(buffer + 11 + len_of_name, "%s", device_price);
        
        long dev_price = valid_price(device_price, CLASS_3); // megvizsgálja, hogy megfelel-e a pénzösszeg a feltételeknek
        if(dev_price > 0)
            p->data.price = dev_price; // bemásolja az új listaelembe az értéket
        else
        {
            free(device_price);
            free(p);
            printf("Price is invalid\n");
            return NULL;
        }
        free(device_price); // nincs szükség tovább a device_price stringre
        p->next = head;
        p->under = NULL;
        head = p;
   }
   fclose(fp);
   if(head == NULL)
   {
       printf("First file is empty\n");
   }
   return head;
}

// paraméterként kap egy készülékazonosítót és megvizsgálja, hogy érvényes-e az adat, ha érvényes akkor a visszatérési érték 0, ha nem akkor 1
int valid_dev_id_len(long id)
{
    if(id / 100000000 > 0 && id / 100000000 < 10) // PL: 123456789 esetén 1 az osztás eredménye
        return 1;
    return 0;
}

// paraméterként kap egy készülékazonosítót és a fésűs lista elejére mutató pointert.
//A függvény végigjárja a fésűs listát és megnézi, hogy talál-e a paraméterlistán kapott azonosítóval megegyező azonosító a listában
// Ha talál akkor a visszatérési érték 1, ha nem akkor 0.
int check_for_existing_device_ID(long id, main_node *head)
{
    main_node *p = head;
    while(p != NULL)
    {
        if(p->data.device_ID == id)
            return 1;
        p = p->next;
    }
    return 0;
}

// paraméterként kap egy számot ábrázoló stringet (PL: "5672") illetve a legdrágább biztosítási csomag alapárát.
// Ha a szám tárolható long-ként akkor árkonvertálja a stringet longgá és visszatér vele, ha nem, akkor 3 eset van: (1): negatív a szám, ekkor -2 a visszatérési érték
// (2): a szám túl nagy és ezért nem fér el egy long változóban, ekkor a visszatétési érték -1 (3): ha a számhoz a maximális csomagárat hozzáadva túlcsordulna akkor
// a visszatérési érték -3
long valid_price(char *str, double max_base_price)
{
    int t[] = {2, 1, 4, 7, 4, 8, 3, 6, 4, 7};  // A legnagyobb long helyiértékei (balszélső a legnagyobb helyiértékű)
    if(strlen(str) > 10) // ha túl hosszú a szám
        return -1;
    
    if(str[0] == '-') // ha negatív
        return -2;
    
    if(strlen(str) == 10)
    {
        for(int i = 0; i < 10; i++)
        {
            if(str[i] > t[i]) // ha pontosan 10 karakter hosszú a szám, akkor egyik számjegye sem lehet nagyobb a t[] tömb adott eleménél
                return -1;
        }
    }

    // ha a fenti teszteken túljutott a függvény, akkor átkonvertálja longgá a stringet
    long result = 0;
    for(int i = 0; i < strlen(str); i++)
    {
        result *= 10;
        result += str[i] - '0';
    }
    if(result + max_base_price < result) // ellenőrzi, hogy a maximális csomagárat hozzáadva a számhoz se csorduljon túl
        return -3;
    return result;
}

// paraméterként megkapja a fésűs lista elejére mutató poinert és a második fájlra mutató pointert
// A fájlból kiolvassa az adatokat, memóriát foglal nekik és eltárolja őket a lista oldalágain
// Visszatérési értékek: 0: Ha nem ütközik problémába 1: Ha a fájl üres -2: Ha túl sok vagy túl kevés adat van egy sorába a fájlnak -3: ha az újonnan eltárolandó ügyfél azonosítója már használatban van
// -4: ha az ügyfélazonosító hossza nem megfelelő -1: ha a biztosítási csomag értéke nem megfelelő vagy pedig ha az ügyfél által választott telefon nem létezik
int read_customer(main_node *head, FILE *fp)
{
    char buffer[200];
    int is_empty = 1;
    while(fgets(buffer, 200, fp) != NULL) // soronként beolvassa az adatokat
    {
        if(argc_each_line_in_file(buffer) != 4) // megszámolja, hogy mennyi adat van az adott sorban, ha ez az érték nem 4 akkor hibával tér vissza
        {
            printf("Too many or not enough data in a line of the customers file\n");
            return -2;
        }
        is_empty = 0;
        int customer_id;
        sscanf(buffer, "%d", &customer_id); // ügyfélazonosító beolvasása az adott sorból
        if(check_for_existing_cusomer_ID(customer_id, head) == 1) // ellenőrzi, hogy létezik-e már a listában az adott azonosító
        {
            printf("Already existing customer ID\n");
            return -3;
        }
        if(valid_customer_id_len(customer_id) == 0) // ellenőrzi az ügyfélazonosító helyességét
        {
            printf("Invalid customer ID\n");
            return -4;
        }

        side_node *p_side = malloc(sizeof(side_node)); // memóriát foglal egy oldalági listaelemnek

        p_side->data.customer_ID = customer_id;


        // az ügyfél nevének feldolgozása
        int len_of_name = 0, i = 8; // 8: az adot sorban a 8. karakternél kezdődik az ügyfél neve, mert előtte áll egy 6 karter hosszú azonosító és 2 space
        while(1) // megszámolja, hogy milyen hosszú az ügyfél neve
        {
            if(buffer[i] == ' ')
                if(buffer[i + 1] == ' ')
                    break;
            len_of_name++;
            i++;
        }
        char customer_name[100];
        memcpy(customer_name, buffer + 8, len_of_name); // az ügyfél nevét bemásolja egy ideiglenes változóba (customer_name[])
        customer_name[len_of_name] = '\0'; // a név végére elhelyezi a string végjelét
        strcpy(p_side->data.name, customer_name); // az oldalági listaelembe bemásolja az ügyfél nevét

        long device_id;
        int ins_type;
        // kiolvassa a buffer stringből a készülékazonosítót és a biztsítási csomagot
        sscanf(buffer + 8 + len_of_name, "%li  %d", &device_id,  &ins_type); // 8 karakter az első azonosító és a 2 db space utána pedig még a név hosszát hozzáadva érünk el oda, ahol a még be nem olvasott adatok kezdődnek
        if(ins_type > 0 && ins_type < 4) // megnézi, hogy mefelel-e a biztosítási csomag
            p_side->data.insurance_type = ins_type;
        else
        {
            free(p_side);
            return -1;
        }

        main_node *p_main = head;
        while(p_main->data.device_ID != device_id) // megkeresi, hogy melyik gerincelem oldalágába kell beszúrni az új oldalági listaelemet
        {
            if(p_main == NULL) // ha elért a gerinc végéig akkor az azt jelenti, hogy nem talált megfelelő oldalágat és ekkor hibával tér vissza
            {
                printf("file processing fail\n");
                return -1;
            }
            p_main = p_main->next;
        }
        // beszúrja az oldalágba az új listaelemet
        p_side->next = p_main->under;
        p_main->under = p_side;
    }

    if(is_empty == 1)
    {
        printf("Customer file is empty\n");
        return 1;
    }
    fclose(fp);
    return 0;
}

// paraméterként kap egy ügyfélazonosítót és megvizsgálja, hogy érvényes-e az adat, ha érvényes akkor a visszatérési érték 0, ha nem akkor 1
int valid_customer_id_len(int id)
{
    if(id / 100000 > 0 && id / 100000 < 10)
        return 1;
    return 0;
}

// paraméterként kap egy ügyfélazonosítót és a fésűs lista elejére mutató pointert.
// A függvény végigjárja a lista oldalágait és ha talál egyező elemet, akkor visszatér 1-es értékkel, ha nem akkor 0 a visszatérési érték
int check_for_existing_cusomer_ID(int id, main_node *head)
{
    main_node *p = head;
    while(p != NULL) // végigjárja a gerincét a fésűs listának
    {
        side_node *side = p->under;
        while(side != NULL) // végigjárja az oldalágakat
        {
            if(side->data.customer_ID == id) // ha talál egyező adatokat akkor hibával tér vissza
                return 1;
            side = side->next;
        }
        p = p->next;
    }
    return 0;
}

// paraméterként kap egy nevet stringként és a fésűs lista elejére mutató pointert. A függvény megnézi, hogy van-e névegyezés az ügyfelek között. A visszatérési érték az adott névvel rendelkező ügyfelek száma
int check_for_same_name(char *name, main_node *head)
{
    int same = 0;
    main_node *p_main = head;
    while(p_main != NULL) // végigjárja a gerincét a fésűs listának
    {
        side_node *p_side = p_main->under;
        while(p_side != NULL) // végigjárja az oldalágakat
        {
            if(strcmpi(p_side->data.name, name) == 0) // megnézi, hogy van-e névegyezés, ha van, akkor növeli az egyezésszámlálót
                same++;
            p_side = p_side->next;
        }
        p_main = p_main->next;
    }
    return same; // visszatérési érték: mennyi egyezést talált
}

// paraméterként kap egy stringet ami egy fájl adott sorát tartalmazza és visszatér az adott sorban található adatok számával
int argc_each_line_in_file(char *string)
{
    int argc_count = 1; // Pélául 3 argumentum esetén csak 2 db dupla space van ezért 1-ről indítjuk a számlálót
    for(int i = 0; string[i] != '\0'; i++)
    {
        if(string[i] == ' ')
            if(string[i + 1] == ' ')
            {
                i++;
                argc_count++;
            }
    }
    return argc_count;
}


// Paraméterként kap egy fésűs listát és megszámlálja, hogy mennyi ügyfél van majd ezzel az értékkel visszatér
// Paraméterlistán visszaadaja, hogy mennyien választották az egyes csomagokat
int customer_count(main_node *head, int *class1_count, int *class2_count, int *class3_count)
{
    int cnt1 = 0, cnt2 = 0, cnt3 = 0;
    int customer_count = 0;
    main_node *p = head;
    while (p != NULL) // végigjárja a gerincet
    {
        side_node *p_side = p->under;
        while (p_side != NULL) // végigjárja az oldalágakat
        {
            // megszámlálja az ügyfeleket és a csomagválasztásokat
            customer_count++;
            if (p_side->data.insurance_type == 1)
                cnt1++;
            else if (p_side->data.insurance_type == 2)
                cnt2++;
            else if (p_side->data.insurance_type == 3)
                cnt3++;
            p_side = p_side->next;
        }
        p = p->next;
    }
    // paraméterlistán visszaadja az eredményeket
    *class1_count = cnt1;
    *class2_count = cnt2;
    *class3_count = cnt3;
    return customer_count;
}

// Paraméterek: hányan választották az egyes csomagokat (1-2-3)
// Kiszámolja, hogy az egyes csomagok árát mennyivel kell csökkenteni és ezeket az eredményeket paraméterlistán adja vissza
void price_optimalization(double *class1_price, double *class2_price, double *class3_price)
{
    if (class1_count * 0.1 > 5)
        *class1_price = CLASS_1 - 5;
    else
        *class1_price = CLASS_1 - class1_count * 0.1;

    if (class2_count * 0.2 > 5)
        *class2_price = CLASS_2 - 5;
    else
        *class2_price = CLASS_2 - class2_count * 0.2;

    if (class3_count * 0.1 > 5)
        *class3_price = CLASS_3 - 5;
    else
        *class3_price = CLASS_3 - class3_count * 0.25;
}

// A visszatérési érték 1, ha az első paraméter előrébb van az abc-ben és 0, ha a második paraméter van előrébb
int compare_strings(char *str1, char *str2)
{
    int min_len = strlen(str1) < strlen(str2) ? strlen(str1) : strlen(str2); // megnézi, hogy melyik string a rövidebb
    for (int i = 0; i < min_len; i++)
    {
        if (tolower(str1[i]) == tolower(str2[i]))
            continue;
        if (tolower(str1[i]) < tolower(str2[i]))
            return 1;
        if (tolower(str2[i]) < tolower(str1[i]))
            return 0;
    }
    if (strlen(str1) < strlen(str2)) // ha nevek megegyeznek, de az egyik rövidebb akkor a rövidebb kerül előrébb a névsorban
        return 1;
    else
        return 0;
}

// első paraméterként kap egy pénzösszeget, második paramétere egy biztosítási csomag típus
// visszatérési értéke -1.0, ha hibás az input, egyébként a biztosítási csomagoknak megefelelő végösszeget adja vissza
double calculate_final_price(int base_price, int ins_class)
{
    // a telefon alapárához hozzáadja a módosított csomagárakat
    if (ins_class == 1)
        return base_price + class1_price;
    else if (ins_class == 2)
        return base_price + class2_price;
    else if (ins_class == 3)
        return base_price + class3_price;
    return -1.0;
}

// Paraméterként megkapja a fésűs listára mutató pointert és az ügyfelek számát. A standard outputra kiírja az ügyfelek számát és azt, hogy az egyes csomagokat hányan választották (golábils változók)
// A listának az elemeit végigjárja és kiírja a megfelelő adatokat a standard outputra (név - fizetendő összeg - csomag típusa)
void print_result(main_node *head, int customer_count)
{
    printf("======================================\n");
    printf("Ugyfelek szama: %d\nBiztositasi csomagok szerinti bontas:\n1-es csomag: %d\n2-es csomag: %d\n3-as csomag: %d\n", customer_count, class1_count, class2_count, class3_count);
    main_node *p_main = head;
    while(p_main != NULL) // végigjárja a gerincet
    {
        side_node *p_side = p_main->under;
        if(p_side != NULL) // ha az oldalágon vannak elemeket akkor kiírja a formai elemeket és az adott telefon nevét
        {
            printf("======================================\n");
            printf("%s\n", p_main->data.name);
        }
        while(p_side != NULL) // végigjárja az oldalágat
        {
            if(check_for_same_name(p_side->data.name, p_main) > 1) // ha vannak azonos nevűek akkor kiírja az ügyfelek neve mellé az ügyfélazonosítókat
            {
                printf("-%s (%d) - %.2f$ - ", p_side->data.name, p_side->data.customer_ID, calculate_final_price(p_main->data.price, p_side->data.insurance_type));
            }
            else
                printf("-%s - %.2f$ - ", p_side->data.name, calculate_final_price(p_main->data.price, p_side->data.insurance_type));
            
            // a magyar nyelv szabályainak megfelelően kiírja a csomagneveket
            if(p_side->data.insurance_type == 3)
                printf("%d-as csomag\n", p_side->data.insurance_type);
            else
                printf("%d-es csomag\n", p_side->data.insurance_type);
            p_side = p_side->next;
        }
        p_main = p_main->next;
    }
    printf("======================================\n");
}

// Paraméterként kap egy fésűs listát és elemenként felszabadítja a listát
void free_list(main_node *head)
{
    main_node *next_main = head, *p_main;
    side_node *next_side, *p_side;

    while(next_main != NULL) // végigjárja a gerincet
    {
        next_side = next_main->under;
        while(next_side != NULL) // végigjárja az oldalágakat
        {
            p_side = next_side->next;
            free(next_side); // felszabadítja egyesével az elemeket az oldalágon
            next_side = p_side;
        }
        p_main = next_main->next;
        free(next_main); // felszabadítja egyesével a gerincet
        next_main = p_main;
    }
}


// Merge sort from GeeksforGeeks

void MergeSort(side_node** headRef)
{
    side_node* head = *headRef;
    side_node *a;
    side_node *b;

    // lekezeli azokat az eseteket ha 0 vagy 1 elem van csak a listában
    if((head == NULL) || head->next == NULL)
        return;
    
    // Felbontja két részlistára 
    FrontBackSplit(head, &a, &b);
    // rekurzívan sorba rendezi a részlistákat
    MergeSort(&a);
    MergeSort(&b);

    // egybeolvasztja a rendezett részlistákat
    *headRef = SortedMerge(a, b);
}

// A függvény részletei: https://www.geeksforgeeks.org/merge-two-sorted-linked-lists/
side_node* SortedMerge(side_node* a, side_node* b)
{
    side_node *result = NULL;

    // alapesetek kezelése
    if(a == NULL)
        return b;
    else if(b == NULL)
        return a;
    
    // a feltételeknek megfelelően kiválasztjuk a-t vagy b-t és újrahívjuk a függvényt
    if(compare_strings(a->data.name, b->data.name) == 1)
    {
        result = a;
        result->next = SortedMerge(a->next, b);
    }
    else
    {
        result = b;
        result->next = SortedMerge(a, b->next);
    }
    return result;
}

// Két külön részre bontja a listát és a keletkező listákat paraméterlistán adja vissza
// Ha páratlan hosszú a lista akkor az extra elem az első listába megy
// A "Fast/Slow pointer strategy"-t hasznnálja
void FrontBackSplit(side_node* source, side_node** frontRef, side_node** backRef)
{
    side_node* fast;
    side_node* slow;
    slow = source;
    fast = source->next;

    // A gyorsabb pointer 2-t lép míg a lassab csak 1-et.
    while(fast != NULL)
    {
        fast = fast->next;
        if(fast != NULL)
        {
            slow = slow->next;
            fast = fast->next;
        }
    }

    // Amikor a gyorsabb elérte a NULL-t pointert akkor a lassab pointer a lista 
    // középpontja előtt van ezért azzal lépünk még egyet és ketté választjuk a listát
    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
}
