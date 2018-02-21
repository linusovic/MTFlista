/*
 * tabletest - test different implementation of a table.
 *
 * Should be compiled together with a table implementation that
 * follows the interface in table.h of the code base for the
 * Datastructures and Algorithms courses at the Department of
 * Computing Science, Umea University.
 *
 * 20xx-xx-xx v1.0 Lena Kallin Westin <kallin@cs.umu.se>.
 * 2017-03-xx v1.1 Adam Dahlstrom <dali@cs.umu.se>
 *                 Modified to use dynamic memory.
 * 2017-05-xx v1.2 Niclas Borlin <niclas@cs.umu.se>.
 *                 Simplified printout and use. Test size can now be
 *                 specified on the command line. Lookup (no key) moved
 *                 before lookup to avoid cache-hit effects on timing of
 *                 mtftable.
 * 2017-05-xx v1.3 Niclas Borlin <niclas@cs.umu.se>.
 *                 Added more fine-grained tests.
 * 2018-02-08 v1.4 Niclas Borlin <niclas@cs.umu.se>.
 *                 Modified to match 2018 implementation of the data
 *                 structure code base.
 * 2018-02-20 v1.5 Niclas Borlin <niclas@cs.umu.se>.
 *                 Now completely destroys and rebuilds table between timed
 *                 tests to reduce cache effects.
*/

#define VERSION "v1.5"
#define VERSION_DATE "2018-02-20"

/*
 * Correctness testing algorithm:
 *
 * 1. Tests if isempty returns true directly after a table is created
 * 2. Tests if isempty returns false directly after a table is created
 *    and one element (key-value-pair) is inserted to it.
 * 3. Tests a table by creating it and inserting one key-value-pair. After
 *    that it is checked that the returned values from a lookup are the ones
 *    expected. First by looking up a non-existent key, then by looking up
 *    an existent key.
 * 4. Tests a table by creating it and inserting three key-value-pairs with
 *    unique keys. After that, a lookup for all three keys are tested and
 *    it is checked that the returned values are the ones expected.
 * 5. Tests a table by creating it and inserting three key-value-pairs with
 *    the same keys. After that, a lookup for the key is tested and it is
 *    checked that the returned value is the last one inserted to the table.
 * 6. Tests a table by creating it and inserting one key-value-pair. After
 *    that the element is removed and it is checked that the table is empty.
 * 7. Tests a table by creating it and inserting three key-value-pairs. After
 *    that the elements is removed one at a time and it is checked that the
 *    table is empty after the third element is removed.
 * 8. Tests a table by creating it and inserting a single key-value
 *    pair followed by three key-value-pairs with identical
 *    keys. After that, the first element is remove and it is verified
 *    that it is gone and that the other key returns the cocorrect
 *    value. The second key is removed and it is checked that the
 *    table is empty.
 *
 * There is also a module measuring time for insertions, lookups etc.
 * */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <table.h>

// Maximum size of the table to generate
#define TABLESIZE 40000
#define SAMPLESIZE TABLESIZE*2

/**
 * copy_string() - Create a dynamic copy of a string.
 * @s: String to be copied.
 *
 * Allocates memory for a dynamic copy of s and copies the contents of
 * s into the copy.
 *
 * Returns: Pointer to the copy of s.
 */
char *copy_string(const char *s)
{
        int len=strlen(s);

        /* Allocate memory for new string, with an extra char for \0 */
        char *dest = malloc(sizeof(char)*(len+1));

        /* Malloc failed, return NULL */
        if (dest == NULL) {
                return NULL;
        }

        /* Copy content to new memory */
        strncpy(dest, s, len);

        /* Strings should always be null terminated */
        dest[len] = '\0';
        return dest;
}

/**
 * int_ptr_from_int() - Create a dynamic copy of an integer.
 * @i: String to be copied.
 *
 * Allocates memory for a dynamic copy of i and copies the contents of
 * i into the copy.
 *
 * Returns: Pointer to the copy of i.
 */
int *int_ptr_from_int(int i)
{
        // Allocate memory for a dynamic copy of an integer
        int *ip=malloc(sizeof(int));
        // Copy the value
        *ip=i;
        return ip;
}

/**
 * get_milliseconds() - Return the current time-of-day in milliseconds.
 *
 * Returns: The current time-of-day in milliseconds.
 */
unsigned long get_milliseconds()
{
        struct timeval tv;
        gettimeofday(&tv, 0);
        return (unsigned long)(tv.tv_sec*1000 + tv.tv_usec/1000);
}

/**
 * int_compare() - Compare to integers via pointers.
 * @ip1, @ip2: Pointers to integers to be compared.
 *
 * Compares the integers that ip1 and ip2 points to.
 *
 * Returns: 0 if the integers are equal, negative if the first
 * argument is smaller, positive if the first argument is larger.
 */
int int_compare(const void *ip1,const void *ip2)
{
        const int *n1=ip1;
        const int *n2=ip2;
        return (*n1 - *n2);
}

/**
 * string_compare() - Compare two strings.
 * @ip1, @ip2: Pointers to strings to be compared.
 *
 * Compares the strings that ip1 and ip2 points to.
 *
 * Returns: 0 if the strings are equal, negative if the first
 * argument is smaller, positive if the first argument is larger.
 */
int string_compare(const void *ip1,const void *ip2)
{
        const char *s1=ip1;
        const char *s2=ip2;
        return strcmp(s1,s2);
}

/* Shuffles the numbers stored in seq
 *    seq - an array of randomnumbers to be shuffled
 *    n - the number of elements in seq to shuffle, i.e the indexes [0, n]
 *        will be shuffled. However, seq might be larger than n...
 */
void random_shuffle(int seq[], int n)
{
        for(int i=0;i<n;i++) {
                int switchPos=rand()%n;
                if(i!=switchPos) {
                        int temp=seq[i];
                        seq[i]=seq[switchPos];
                        seq[switchPos]=temp;
                }
        }
}

/* Generate n unique random numbers. The numbers will be stored i seq
 *    seq - an array of randomnumbers created in the function
 *    n - the number of elements in seq
 */
void create_random_sample(int seq[], int n)
{
        for(int i=0;i<n;i++) {
                seq[i]=i;
        }
        random_shuffle(seq,n);
}


/* Fill a table with values.
 *    table - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 */
void insert_values(table *t, int *keys, int *values, int n)
{
        // Insert all items
        for(int i=0;i<n;i++) {
                table_insert(t, int_ptr_from_int(keys[i]),
                             int_ptr_from_int(values[i]));
        }
}


/* Measures time taken to fill a table with values
 *    table - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 */
void get_insert_speed(table *t, int *keys, int *values, int n)
{
        unsigned long start;
        unsigned long end;

        // Insert all items
        printf("Insert %5d items                   : ", n);
        start = get_milliseconds();
        insert_values(t,keys,values,n);
        end =  get_milliseconds();
        printf("%lu ms.\n",end-start);
}

/* Measures time taken to do n lookups of existing keys in a table
 *    t - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 *    n - the number of lookups to perform
 */
void get_random_existing_lookup_speed(table *t, int *keys, int n)
{
        unsigned long start;
        unsigned long end;

        printf("%5d random lookups                 : ",n );
        start = get_milliseconds();
        for(int i=0;i<n;i++) {
                // The existing keys in the table are stored in index
                // [0, TABLESIZE-1] in the key-array
                int pos = rand()%n;
                table_lookup(t,&keys[pos]);
        }
        end = get_milliseconds();
        printf("%lu ms.\n", end-start);
}

/* Measures time taken to do n lookups of non-existing keys in a table
 *    t - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 *    n - the number of lookups to perform
 */
void get_random_non_existing_lookup_speed(table *t, int *keys, int n)
{
        unsigned long start;
        unsigned long end;

        printf("%5d lookups with non-existent keys : ", n);
        // We know the exisiting keys have indexes in [0, TABLESIZE-1]
        // so if we try to lookup keys in the area [TABLESIZE,
        // 2*TABLESIZE-1] they will not exist
        start =  get_milliseconds();
        int startindex = n;
        for(int i=0;i<n;i++){
                table_lookup(t,&keys[startindex + (i%n)]);
        }
        end = get_milliseconds();
        printf("%lu ms.\n" ,end-start);
}

/* Measures time taken to do n lookups of existing keys in a table when the
 * keys chosen are from only a part of all available keys
 *    t - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 *    n - the number of lookups to perform
 */
void get_skewed_lookup_speed(table *t, int *keys, int n)
{
        unsigned long start;
        unsigned long end;

        // Lookup skewed to a certain range (in this case the middle third
        // of the keys used)
        int startindex = n/3;
        int stopindex = n*2/3;
        int partition = stopindex - startindex + 1;

        printf("%5d skewed lookups                 : " ,n);
        start =  get_milliseconds();
        for(int i=0;i<n;i++) {
                int pos = rand()%partition + startindex;
                table_lookup(t,&keys[pos]);
        }
        end = get_milliseconds();
        printf("%lu ms.\n" ,end-start);
}

/* Measures time taken remove all keys from a table
 *    t - the table to fill
 *    keys - a list of keys to use
 *    values - a list of values to use
 */
void get_remove_speed(table *t, int *keys, int n)
{
        unsigned long start;
        unsigned long end;

        // Remove all items, not in the same order as they were inserted
        printf("Remove all items                     : ");
        start = get_milliseconds();
        random_shuffle(keys, n);  // to get a new order of the keys
        for(int i=0;i<n;i++) {
                table_remove(t,&keys[i]);
        }
        end = get_milliseconds();
        printf("%lu ms.\n" ,end-start);
}

/* Tests if isempty returns true directly after a table is created.
 */
void test_isempty(){
        table *t = table_empty(int_compare, NULL, NULL);

        if (!table_is_empty(t)){
                printf("An newly created empty table is said to be nonempty.\n");
                exit(EXIT_FAILURE);
        }
        printf("Isempty returns true directly after a table is created. - OK\n");
        table_kill(t);
}

/* Tests if isempty returns false directly after a table is created
 * and one element (key-value-pair) is inserted to it.
 */
void test_insert_single_element(void)
{
        table *t = table_empty(string_compare, free, free);
        char *key = copy_string("key1");
        char *value = copy_string("value1");

        table_insert(t, key, value);
        if (table_is_empty(t)){
                printf("A table with one inserted element is seen as empty.\n");
                exit(EXIT_FAILURE);
        }
        printf("Isempty false if one element is inserted to table. - OK\n");
        table_kill(t);
}

/* Tests looking up the key key in a table table. Checks that lookup is
 *  returning the value value
 *     t - the table to do the lookup in
 *     key - the key to lookup
 *     value - the expected value
 */
void test_lookup_existing_key(table *t, char *key, char *value)
{
        char *return_value = (char *)table_lookup(t, key);
        if (return_value==NULL){
                printf("Looked up an existing key, table claims it does "
                       "not exist.\n");
                exit(EXIT_FAILURE);
        }
        if (strcmp(return_value, value)){
                printf("Looked up a key but the value returned was wrong.");
                printf(" Expected: %s but got %s.\n", value, return_value);
                exit(EXIT_FAILURE);
        }
}

/* Tests looking up the key key in a table table. Verifies that the
 * key is not found.
 */
void test_lookup_missing_key(table *t, char *key)
{
        char *return_value = (char *)table_lookup(t, key);
        if (return_value!=NULL){
                printf("Looked up an missing key %s, table claims it has "
                       "value %s.\n",key,return_value);
                exit(EXIT_FAILURE);
        }
}

/*  Tests a table by creating it and inserting one key-value-pair. After
 *  that it is checked that the returned values from a lookup are the ones
 *  expected. First by looking up a non-existent key, then by looking up
 *  an existent key.
 *
 *  It is assumed that test_insert_single_value has been run before calling
 *  this test, i.e. that it has been tested that inserting one element
 *  results in an nonempty table.
 */
void test_lookup_single_element()
{
        table *t = table_empty(string_compare, free, free);

        char *key1 = copy_string("key1");
        char *value1 = copy_string("value1");
        char *value2;

        table_insert(t, key1, value1);

        // Lookup a non-existent key
        value2 = (char *)table_lookup(t, "key2");
        if (value2!=NULL){
                printf("Looked up non-existing key, table claims it does "
                       "exist.\n");
                exit(EXIT_FAILURE);
        }
        printf("Test of looking up non-existing key in a table with one "
               "element - OK\n");

        // Lookup an existent key
        test_lookup_existing_key(t, key1, value1);
        printf("Looking up existing key in a table with one element - OK\n");
        table_kill(t);
}

/* Tests a table by creating it and inserting three key-value-pairs with
 *  unique keys. After each step, all existing keys are looked up and
 *  it is checked that the returned values are the ones expected.
 *
 *  It is assumed that test_insert_single_value and test_lookup_single_value
 *  has been run before calling this test, i.e. that it has been tested
 *  that inserting one element results in an nonempty table and that
 *  looking up an key-value-pairs is working with tablesize=1.
 */
void test_insert_lookup_different_keys()
{
        table *t = table_empty(string_compare, free, free);

        char *key1 = copy_string("key1");
        char *key2 = copy_string("key2");
        char *key3 = copy_string("key3");
        char *value1 = copy_string("value1");
        char *value2 = copy_string("value2");
        char *value3 = copy_string("value3");

        table_insert(t, key1, value1);
        test_lookup_existing_key(t, "key1", "value1");
        table_insert(t, key2, value2);
        test_lookup_existing_key(t, "key1", "value1");
        test_lookup_existing_key(t, "key2", "value2");
        table_insert(t, key3, value3);
        test_lookup_existing_key(t, "key1", "value1");
        test_lookup_existing_key(t, "key2", "value2");
        test_lookup_existing_key(t, "key3", "value3");

        printf("Looking up three existing keys-value pairs in a table "
               "with three elements - OK\n");
        table_kill(t);
}

/* Tests a table by creating it and inserting three key-value-pairs with
 *  the same keys. After each insert, the new key is looked up and it is
 *  checked that the returned value is the last one inserted to the table.
 *
 *  has been run before calling this test, i.e. that it has been tested
 *  It is assumed that test_insert_single_value and test_lookup_single_value
 *  that inserting one element results in an nonempty table and that
 *  looking up an key-value-pairs is working with tablesize=1.
 *
 */
void test_insert_lookup_same_keys()
{
        table *t = table_empty(string_compare, free, free);

        /* Separate key to use on lookup, since it is not defined
         * which duplicate key will be removed.
         */
        char *key = "key1";
        /* Different allocations for keys, but same content. Behaviour
         * is undefined if the same pointer is used multiple times.
         * Memhandler will break things in that case. */
        char *key1 = copy_string(key);
        char *key2 = copy_string(key);
        char *key3 = copy_string(key);

        char *value1 = copy_string("value1");
        char *value2 = copy_string("value2");
        char *value3 = copy_string("value3");

        table_insert(t, key1, value1);
        test_lookup_existing_key(t, key, value1);
        table_insert(t, key2, value2);
        test_lookup_existing_key(t, key, value2);
        table_insert(t, key3, value3);
        test_lookup_existing_key(t, key, value3);

        printf("Looking up existing key and value after inserting the same "
               "key three times with different values - OK\n");
        table_kill(t);
}

/* Tests a table by creating it and inserting one key-value-pair. After
 *  that the element is removed and it is checked that the table is empty.
 *
 *  It is assumed that test_insert_single_value has been run before calling
 *  this test, i.e. that it has been tested that inserting one element
 *  results in an nonempty table.
 */
void test_remove_single_element()
{
        table *t = table_empty(string_compare, free, free);

        char *key1 = copy_string("key1");
        char *value1 = copy_string("value1");

        table_insert(t, key1, value1);
        table_remove(t, key1);
        if (!table_is_empty(t)){
                printf("Removing the last element from a table does not "
                       "result in an empty table.\n");
                exit(EXIT_FAILURE);
        }
        printf("Inserting one element and removing it, checking that the "
               "table gets empty - OK\n");
        table_kill(t);
}

/* Tests a table by creating it and inserting three key-value-pairs.
 *  After that the elements is removed one at a time. After each
 *  removal, it is verified that each removed key is indeed missing
 *  and that each remaining key is still there. Finally, it is tested
 *  that the table is empty after the third element has been removed.
 *
 *  It is assumed that test_insert_single_value and test_remove_single_value
 *  have been run before calling this test, i.e. that it has been tested
 *  that inserting one element results in an nonempty table and removing
 *  one element from a table with one element results in an empty table.
 */
void test_remove_elements_different_keys()
{
        table *t = table_empty(string_compare, free, free);

        char *key1 = copy_string("key1");
        char *key2 = copy_string("key2");
        char *key3 = copy_string("key3");
        char *value1 = copy_string("value1");
        char *value2 = copy_string("value2");
        char *value3 = copy_string("value3");

        table_insert(t, key1, value1);
        table_insert(t, key2, value2);
        table_insert(t, key3, value3);

        table_remove(t, key1);
        if (table_is_empty(t)){
                printf("Should be two elements left in the table but it says "
                       "it is empty\n");
                exit(EXIT_FAILURE);
        }
        test_lookup_missing_key(t, "key1");
        test_lookup_existing_key(t, "key2", "value2");
        test_lookup_existing_key(t, "key3", "value3");

        table_remove(t, key2);
        if (table_is_empty(t)){
                printf("Should be one element left in the table but it "
                       "says it is empty\n");
                exit(EXIT_FAILURE);
        }
        test_lookup_missing_key(t, "key1");
        test_lookup_missing_key(t, "key2");
        test_lookup_existing_key(t, "key3", "value3");

        table_remove(t, key3);
        if (!table_is_empty(t)){
                printf("Removing the last element from a table does not "
                       "result in an empty table.\n");
                exit(EXIT_FAILURE);
        }
        test_lookup_missing_key(t, "key1");
        test_lookup_missing_key(t, "key2");
        test_lookup_missing_key(t, "key3");

        printf("Inserting three elements and removing them, should end with "
               "empty table - OK\n");
        table_kill(t);
}

/* Tests a table by creating it and inserting a single key-value
 *  follow by a set of three key-value-pairs with duplicate keys. The
 *  first key is removed, and it is tested that the key is gone and
 *  that the second key gives the right result. Then the second key is
 *  removed is it is checked that the table is empty.
 *
 *  It is assumed that test_insert_single_value and test_remove_single_value
 *  have been run before calling this test, i.e. that it has been tested
 *  that inserting one element results in an nonempty table and removing
 *  one element from a table with one element results in an empty table.
 */
void test_remove_elements_same_keys()
{
        table *t = table_empty(string_compare, free, free);

        /* Separate key to use in remove, since it is not defined
         * which duplicate key will be removed.
         */
        char *key1 = "key1";
        char *key2 = "key2";
        /* Different allocations for keys, but same content. Behaviour
         * is undefined if the same pointer is used multiple times.
         * Memhandler will break things in that case. */
        char *key11 = copy_string(key1);
        char *key21 = copy_string(key2);
        char *key22 = copy_string(key2);
        char *key23 = copy_string(key2);

        char *value11 = copy_string("value11");
        char *value21 = copy_string("value21");
        char *value22 = copy_string("value22");
        char *value23 = copy_string("value23");

        /* Insert triplicates and verify that each new value indeed
         * becomes the new value. */
        table_insert(t, key11, value11);

        /* Insert another set of triplicates and verify that each new
         * value indeed becomes the new value. */
        table_insert(t, key21, value21);
        table_insert(t, key22, value22);
        table_insert(t, key23, value23);

        /* Now remove the first element */
        table_remove(t, key1);
        test_lookup_missing_key(t, key1);
        /* This will fail if remove permutes the order of the
           duplicate key2 values */
        test_lookup_existing_key(t, key2, "value23");

        table_remove(t, key2);
        test_lookup_missing_key(t, key2);
        if (!table_is_empty(t)) {
                printf("Removing the last element from a table does not "
                       "result in an empty table.\n");
                exit(EXIT_FAILURE);
        }

        printf("Inserting three elements with the same key and removing "
               "the key, should end with empty table - OK\n");
        table_kill(t);
}

/*  Tests a table by performing a set of tests. Program exits if any
 *  error is found.
 */
void correctnessTest()
{
        test_isempty();
        test_insert_single_element();
        test_lookup_single_element();
        test_insert_lookup_different_keys();
        test_insert_lookup_same_keys();
        test_remove_single_element();
        test_remove_elements_different_keys();
        test_remove_elements_same_keys();
}

/* Tests the speed of a table using random numbers. First a number of
 * elements are inserted. Second a random lookup among the elements are
 * done followed by a skewed lookup (where a subset of the keys are
 * looked up more frequently). Finally all elements are removed.
 */
void speedTest(int n)
{
        int randomsize = 2*n; // To make it easier testing
                              // non-existing keys later
        int *keys = malloc(randomsize*sizeof(int));
        int *values = malloc(randomsize*sizeof(int));
        create_random_sample(keys, randomsize);
        create_random_sample(values, n);

        table *t = table_empty(int_compare, free, free);
        get_insert_speed(t, keys, values, n);
        table_kill(t);

        t = table_empty(int_compare, free, free);
        insert_values(t,keys,values,n);
        get_remove_speed(t, keys, n);
        table_kill(t);

        t = table_empty(int_compare, free, free);
        insert_values(t,keys,values,n);
        get_random_non_existing_lookup_speed(t, keys, n);
        table_kill(t);

        t = table_empty(int_compare, free, free);
        insert_values(t,keys,values,n);
        get_random_existing_lookup_speed(t, keys, n);
        table_kill(t);

        t = table_empty(int_compare, free, free);
        insert_values(t,keys,values,n);
        get_skewed_lookup_speed(t, keys, n);
        table_kill(t);

        free(keys);
        free(values);
}

#define NAME "tabletest"

int main(int argc,char **argv)
{
        int n=0;
        fprintf(stderr,NAME " " VERSION "\n");
        if (argc<2) {
                fprintf(stderr,"Usage:\n\t%s n\n\twhere n is an integer from "
                        "1 to %d.\n",argv[0],TABLESIZE);
                n=TABLESIZE;
                fprintf(stderr,"No n supplied, using %d.\n",n);
        } else {
                sscanf(argv[1],"%d",&n);
        }
        if (n<1 || n>TABLESIZE) {
                fprintf(stderr,"Error: supplied value of n (%d) is outside "
                        "allowed range 1-%d.\n",n,TABLESIZE);
                exit(EXIT_FAILURE);
        }
        correctnessTest();
        printf("All correctness tests succeeded!\n\n");
        /*getchar();*/
        speedTest(n);
        printf("Test completed.\n");
        return 0;
}
