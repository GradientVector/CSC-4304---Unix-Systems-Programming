
#include <stdio.h>

struct data {
    int field1;
    int field2;
    int field3;
};

void add_fields(struct data* dataset);

main()
{
    //create and initialize the structure
    struct data theData;
    theData.field1 = 10;
    theData.field2 = 20;
    theData.field3 = 100;

    //print the initialized values
    printf("Scructure Values:\n");
    printf("Field1: %d\n", theData.field1);
    printf("Field2: %d\n", theData.field2);
    printf("Field3: %d\n", theData.field3);

    //call the add_fields method
    add_fields(&theData);

    //print the values after the add_fields call
    printf("Structure Vales After Call to add_fields():\n");
    printf("Field1: %d\n", theData.field1);
    printf("Field2: %d\n", theData.field2);
    printf("Field3: %d\n", theData.field3);

    return 0;
}

void add_fields(struct data* dataset)
{
    dataset->field3 = dataset->field1 + dataset->field2;
}
