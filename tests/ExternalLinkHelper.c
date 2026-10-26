extern int __ada_test_external_middle(void);

int __ada_test_external_answer(void)
{
    return __ada_test_external_middle() + 1;
}
