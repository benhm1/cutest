#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "CuTest.h"

static int __expecting_assert_fail = 0;
static jmp_buf __assert_failed_unexpected;
static jmp_buf __assert_failed_expected;

void assert(int condition) {

  if (!condition) {
    if (__expecting_assert_fail) {
      longjmp(__assert_failed_expected, 1);
    } else {
      printf("Unexpected assertion failure!\n");
      longjmp(__assert_failed_unexpected, 1);
    }
  }
  return;
}


/*-------------------------------------------------------------------------*
 * CuStr
 *-------------------------------------------------------------------------*/

char* CuStrAlloc(int size)
{
	char* newStr = (char*) malloc( sizeof(char) * (size) );
	return newStr;
}

char* CuStrCopy(const char* old)
{
	int len = strlen(old);
	char* newStr = CuStrAlloc(len + 1);
	strcpy(newStr, old);
	return newStr;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

void CuStringInit(CuString* str)
{
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (char*) malloc(sizeof(char) * str->size);
	str->buffer[0] = '\0';
}

CuString* CuStringNew(void)
{
	CuString* str = (CuString*) malloc(sizeof(CuString));
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (char*) malloc(sizeof(char) * str->size);
	str->buffer[0] = '\0';
	return str;
}

void CuStringDelete(CuString *str)
{
        if (!str) return;
        free(str->buffer);
        free(str);
}

void CuStringResize(CuString* str, int newSize)
{
	str->buffer = (char*) realloc(str->buffer, sizeof(char) * newSize);
	str->size = newSize;
}

void CuStringAppend(CuString* str, const char* text)
{
	int length;

	if (text == NULL) {
		text = "NULL";
	}

	length = strlen(text);
	if (str->length + length + 1 >= str->size)
		CuStringResize(str, str->length + length + 1 + STRING_INC);
	str->length += length;
	strcat(str->buffer, text);
}

void CuStringAppendChar(CuString* str, char ch)
{
	char text[2];
	text[0] = ch;
	text[1] = '\0';
	CuStringAppend(str, text);
}

void CuStringAppendFormat(CuString* str, const char* format, ...)
{
	va_list argp;
	char buf[HUGE_STRING_LEN];
	va_start(argp, format);
	vsprintf(buf, format, argp);
	va_end(argp);
	CuStringAppend(str, buf);
}

void CuStringInsert(CuString* str, const char* text, int pos)
{
	int length = strlen(text);
	if (pos > str->length)
		pos = str->length;
	if (str->length + length + 1 >= str->size)
		CuStringResize(str, str->length + length + 1 + STRING_INC);
	memmove(str->buffer + pos + length, str->buffer + pos, (str->length - pos) + 1);
	str->length += length;
	memcpy(str->buffer + pos, text, length);
}

/*-------------------------------------------------------------------------*
 * CuTest
 *-------------------------------------------------------------------------*/

void CuTestInit(CuTest* t, const char* name, TestFunction function)
{
	t->name = CuStrCopy(name);
	t->failed = 0;
	t->ran = 0;
        t->message = NULL;
	t->function = function;
}

CuTest* CuTestNew(const char* name, TestFunction function)
{
	CuTest* tc = CU_ALLOC(CuTest);
	CuTestInit(tc, name, function);
	return tc;
}

void CuTestDelete(CuTest *t)
{
        if (!t) return;
        CuStringDelete(t->message);
        free(t->name);
        free(t);
}

void CuTestRun(CuTest* tc)
{
	if (setjmp(__assert_failed_unexpected) == 0)
	{
	  tc->ran = 1;
	  (tc->function)(tc);
	}
	else {
	  tc->failed = 1;
	  CuFail_Line(tc, "file", 123, NULL, "unexpected assertion fail");
	  //printf("In else case of cuTestRun\n");
	}
}

static void CuFailInternal(CuTest* tc, const char* file, int line, CuString* string)
{
	char buf[HUGE_STRING_LEN];

	//sprintf(buf, "%s:%d: ", file, line);
	CuStringInsert(string, buf, 0);

	tc->failed = 1;
        free(tc->message);
        tc->message = CuStringNew();
        CuStringAppend(tc->message, string->buffer);
}

void CuFail_Line(CuTest* tc, const char* file, int line, const char* message2, const char* message)
{
	CuString string;

	CuStringInit(&string);
	if (message2 != NULL) 
	{
		CuStringAppend(&string, message2);
		CuStringAppend(&string, ": ");
	}
	CuStringAppend(&string, message);
	CuFailInternal(tc, file, line, &string);
}

void CuAssert_Line(CuTest* tc, const char* file, int line, const char* message, int condition)
{
	if (condition) return;
	CuFail_Line(tc, file, line, NULL, message);
}

void CuAssertStrEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	const char* expected, const char* actual)
{
	CuString string;
	if ((expected == NULL && actual == NULL) ||
	    (expected != NULL && actual != NULL &&
	     strcmp(expected, actual) == 0))
	{
		return;
	}

	CuStringInit(&string);
	if (message != NULL) 
	{
		CuStringAppend(&string, message);
		CuStringAppend(&string, ": ");
	}
	CuStringAppend(&string, "expected <");
	CuStringAppend(&string, expected);
	CuStringAppend(&string, "> but was <");
	CuStringAppend(&string, actual);
	CuStringAppend(&string, ">");
	CuFailInternal(tc, file, line, &string);
}

void CuAssertIntEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	int expected, int actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected <%d> but was <%d>", expected, actual);
	CuFail_Line(tc, file, line, message, buf);
}

void CuAssertDblEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	double expected, double actual, double delta)
{
	char buf[STRING_MAX];
	if (fabs(expected - actual) <= delta) return;
	sprintf(buf, "expected <%f> but was <%f>", expected, actual); 

	CuFail_Line(tc, file, line, message, buf);
}

void CuAssertPtrEquals_LineMsg(CuTest* tc, const char* file, int line, const char* message, 
	void* expected, void* actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected pointer <0x%p> but was <0x%p>", expected, actual);
	CuFail_Line(tc, file, line, message, buf);
}

void CuAssertIntArrayEquals_LineMsg(CuTest* tc,
				    const char* file,
				    int line,
				    const char* message,
				    int* expected,
				    int* actual,
				    size_t length) {
  size_t i;
  char buf[STRING_MAX];
  
  for (i = 0; i < length; i++) {
    if (expected[i] != actual[i]) {
      snprintf(buf, STRING_MAX,
	       "at index %lu expected <%d> but was <%d>",
	       i, expected[i], actual[i]);
      CuFail_Line(tc, file, line, message, buf);
      break;
    }
  }
}

void CuAssertStructEquals_LineMsg(CuTest* tc,
				  const char* file,
				  int line,
				  const char* message,
				  size_t arrayIndex,
				  void* expected,
				  void* actual,
				  size_t sizeofElem,
				  char * (*toString)(void *)) {

  char buf[STRING_MAX];

  if (0 != memcmp(expected, actual, sizeofElem)) {
    if (arrayIndex == SIZE_MAX) {
      snprintf(buf, STRING_MAX,
	       "expected <%s> but was <%s>",
	       toString(expected), toString(actual));
    }
    else {
      snprintf(buf, STRING_MAX,
	       "at array index %lu expected <%s> but was <%s>",
	       arrayIndex,
	       toString(expected), toString(actual));
    }
    CuFail_Line(tc, file, line, message, buf);
  }
  
}
				  
void CuAssertStructArrayEquals_LineMsg(CuTest* tc,
				       const char* file,
				       int line,
				       const char* message,
				       void* expected,
				       void* actual,
				       size_t numElems,
				       size_t sizeofElem,
				       char * (*toString)(void *)) {
  size_t i;
  char buf[STRING_MAX];
  void *expectedIter = expected;
  void *actualIter = actual;
  
  for (i = 0; i < numElems; i++) {
    if (tc->failed) {
      break;
    }
    CuAssertStructEquals_LineMsg(tc, file, line, message,
				 i, expectedIter, actualIter,
				 sizeofElem, toString);
  }
  
}


void CuAssertRaises_Line(CuTest* tc,
			 const char* file,
			 int line,
			 const char* message,
			 void (*fn)(void)) {
  char buf[STRING_MAX] = {0};
  int rv = setjmp(__assert_failed_expected);
  if (!rv) {
    __expecting_assert_fail = 1;
    fn();
    CuFail_Line(tc, file, line, message,
		"expected assertion failure but didn't get one");
  }
  __expecting_assert_fail = 0;
}

/*-------------------------------------------------------------------------*
 * CuSuite
 *-------------------------------------------------------------------------*/

void CuSuiteInit(CuSuite* testSuite)
{
	testSuite->count = 0;
	testSuite->failCount = 0;
        memset(testSuite->list, 0, sizeof(testSuite->list));
}

CuSuite* CuSuiteNew(void)
{
	CuSuite* testSuite = CU_ALLOC(CuSuite);
	CuSuiteInit(testSuite);
	return testSuite;
}

void CuSuiteDelete(CuSuite *testSuite)
{
        unsigned int n;
        for (n=0; n < MAX_TEST_CASES; n++)
        {
                if (testSuite->list[n])
                {
                        CuTestDelete(testSuite->list[n]);
                }
        }
        free(testSuite);

}

void CuSuiteAdd(CuSuite* testSuite, CuTest *testCase)
{
	assert(testSuite->count < MAX_TEST_CASES);
	testSuite->list[testSuite->count] = testCase;
	testSuite->count++;
}

void CuSuiteAddSuite(CuSuite* testSuite, CuSuite* testSuite2)
{
	int i;
	for (i = 0 ; i < testSuite2->count ; ++i)
	{
		CuTest* testCase = testSuite2->list[i];
		CuSuiteAdd(testSuite, testCase);
	}
}

void CuSuiteRun(CuSuite* testSuite)
{
	int i;
	CuString *output = CuStringNew();
	
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		CuTest* testCase = testSuite->list[i];
		CuTestRun(testCase);
		CuStringAppendFormat(output, "%d: %-35s %s\n", i + 1, testCase->name,
				     (testCase->failed ? "FAIL" : "PASS"));
		if (testCase->failed) {
		  testSuite->failCount += 1;
		  CuStringAppendFormat(output, "\t%s\n", testCase->message->buffer);
		}
	}

	//CuSuiteSummary(testSuite, output);
	//CuSuiteDetails(testSuite, output);
	printf("%s\n", output->buffer);  
}

void CuSuiteSummary(CuSuite* testSuite, CuString* summary)
{
	int i;
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		CuTest* testCase = testSuite->list[i];
		CuStringAppend(summary, testCase->failed ? "F" : ".");
	}
	CuStringAppend(summary, "\n\n");
}

void CuSuiteDetails(CuSuite* testSuite, CuString* details)
{
	int i;
	int failCount = 0;

	if (testSuite->failCount == 0)
	{
		int passCount = testSuite->count - testSuite->failCount;
		const char* testWord = passCount == 1 ? "test" : "tests";
		CuStringAppendFormat(details, "OK (%d %s)\n", passCount, testWord);
	}
	else
	{
		if (testSuite->failCount == 1)
			CuStringAppend(details, "There was 1 failure:\n");
		else
			CuStringAppendFormat(details, "There were %d failures:\n", testSuite->failCount);

		for (i = 0 ; i < testSuite->count ; ++i)
		{
			CuTest* testCase = testSuite->list[i];
			if (testCase->failed)
			{
				failCount++;
				CuStringAppendFormat(details, "- %s: %s\n",
                                                     testCase->name, testCase->message->buffer);
			}
		}
		CuStringAppend(details, "\n!!!FAILURES!!!\n");

		CuStringAppendFormat(details, "Runs: %d ",   testSuite->count);
		CuStringAppendFormat(details, "Passes: %d ", testSuite->count - testSuite->failCount);
		CuStringAppendFormat(details, "Fails: %d\n",  testSuite->failCount);
	}
}
