#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PFMT "%u"
typedef unsigned int hval_type;

typedef hval_type (*fhash)(const unsigned char * data, hval_type len);
typedef struct hash_node {
	const char * name;
	const char * descr;
	fhash hash;
} hash_node;

static const char acc_name[] = "acc";
static const char acc_descr[] = "hash += data[i]";
hval_type acc(const unsigned char * data, hval_type len)
{
	hval_type hash = 0;
	for (hval_type i = 0; i < len; ++i)
		hash += data[i];
	return hash;
}

static const char acc_mul_name[] = "acc_mul";
static const char acc_mul_descr[] = "hash += data[i] * i";
hval_type acc_mul(const unsigned char * data, hval_type len)
{
	hval_type hash = 0;
	for (hval_type i = 0; i < len; ++i)
		hash += data[i] * i;
	return hash;
}

static const char jenk_name[] = "jenkins";
static const char jenk_descr[] = "jenkins_one_at_a_time_hash";
hval_type jenkins_one_at_a_time_hash(const unsigned char * data, hval_type len)
{
	hval_type hash = 0;
	for (const unsigned char * pch = data, * end = data+len; pch < end; ++pch)
	{
		hash += *pch;
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash;
}

static const char djb2_name[] = "djb2";
static const char djb2_descr[] = "hash = ((hash << 5) + hash) + c;";
hval_type djb2(const unsigned char * data, hval_type len)
{
	hval_type hash = 5381;
	for (const unsigned char * pch = data, * end = data+len; pch < end; ++pch)
		hash = ((hash << 5) + hash) + *pch; /* hash * 33 + val */
	return hash;
}

static const char sdbm_name[] = "sdbm";
static const char sdbm_descr[] =
	"hash = c + (hash << 6) + (hash << 16) - hash;";
hval_type sdbm(const unsigned char * data, hval_type len)
{
	hval_type hash = 0;
	for (const unsigned char * pch = data, * end = data+len; pch < end; ++pch)
		hash = *pch + (hash << 6) + (hash << 16) - hash;
	return hash;
}

static const char dek_name[] = "dek";
static const char dek_descr[] = "hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);";
hval_type dek(const unsigned char * data, hval_type len)
{
	hval_type hash = 0;
	for (const unsigned char * pch = data, * end = data+len; pch < end; ++pch)
		hash = ((hash << 5) ^ (hash >> 27)) ^ (*pch);
	return hash;
}

hash_node * get_hashes(hval_type * out_len)
{
	static hash_node hashes[] = {
		{acc_name, acc_descr, acc},
		{acc_mul_name, acc_mul_descr, acc_mul},
		{jenk_name, jenk_descr, jenkins_one_at_a_time_hash},
		{djb2_name, djb2_descr, djb2},
		{sdbm_name, sdbm_descr, sdbm},
		{dek_name, dek_descr, dek},
	};
	
	*out_len = sizeof(hashes)/sizeof(*hashes);
	return hashes;
}

#define print_use() \
printf("Use: %s [option] <input-file> <hash-name>\n", prog_name)

const char prog_name[] = "hash-test";
void print_use_quit(void)
{
	print_use();
	printf("Try: %s --help\n", prog_name);
	exit(EXIT_FAILURE);
}

const char opt_help[] = "--help";
const char opt_hashes[] = "--hashes";

#define nl() putchar('\n')
void print_help_quit(void)
{
	printf("%s -- hash function test driver\n", prog_name);
	nl();
	print_use();
	nl();
	puts("Runs each line of <input-file> through <hash-name>; prints");
	puts("<string>    <hash>");
	nl();
	puts("Options:");
	printf("    %s - print the list of supported hash functions\n", opt_hashes);
	printf("    %s   - print this screen\n", opt_help);
	exit(EXIT_SUCCESS);
}
void print_hash_quit(void)
{
	hval_type len = 0;
	hash_node * hashes = get_hashes(&len);
	
	hash_node * hash = NULL;
	for (hval_type i = 0; i < len; ++i)
	{
		hash = hashes+i;
		printf("%s -- %s\n", hash->name, hash->descr);
	}
	exit(EXIT_SUCCESS);
}

void test_hash(const char * fname, const char * hash_name)
{
#define BUFF_SZ 4096

	static char buff[BUFF_SZ] = {0};
	
	hval_type len = 0;
	hash_node * hashes = get_hashes(&len);
	
	hash_node * hash = NULL;
	for (hval_type i = 0; i < len; ++i)
	{
		hash = hashes+i;
		if (strcmp(hash->name, hash_name) == 0)
			break;
		hash = NULL;
	}
	
	if (!hash)
	{
		fprintf(stderr, "%s: error: can't find hash '%s'\n",
			prog_name, hash_name);
		exit(EXIT_FAILURE);
	}
	
	FILE * pf = fopen(fname, "r");
	if (!pf)
	{
		fprintf(stderr, "%s: error: can't open file '%s'\n", prog_name, fname);
		exit(EXIT_FAILURE);
	}
	
	while (fgets(buff, BUFF_SZ, pf))
	{
		len = strlen(buff);
		if ('\n' == buff[len-1])
			buff[len-1] = '\0';
			
		printf("%s    "PFMT"\n", buff,
			hash->hash((const unsigned char *)buff, --len));
	}
	
#undef BUFF_SZ	
}

int main(int argc, char * argv[])
{
	if (argc < 2)
		print_use_quit();
	
	const char * first_arg = argv[1];
	if (strcmp(first_arg, opt_help) == 0)
		print_help_quit();
	else if (strcmp(first_arg, opt_hashes) == 0)
		print_hash_quit();
	else if (3 == argc)
	{
		const char * fname = argv[1];
		const char * hash = argv[2];
		test_hash(fname, hash);
	}
	else
		print_use_quit();
	
	return 0;
}
