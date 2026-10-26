extern int __ada_test_external_tail(void);

int __ada_test_external_middle(void)
{
    return __ada_test_external_tail() + 1;
}
