// Tests: recursive function — caller is the function itself
int recTarget(int n)
{
    if(n <= 0) return 0;
    return recTarget(n - 1);
}
