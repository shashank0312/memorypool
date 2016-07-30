# memorypool
Usage:
class A:public MemPool::PooledObject<>
{
private:
	char *ptr;

	int a; int b;

public:
	A()
	{
		std::cout << "Inside A constructor" << std::endl;
		ptr = new char;
	}
};

class B:public MemPool::PooledObject<>
{
public:
	B()
	{
		std::cout << "Inside B constructor" << std::endl;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	A *a[16];
	for ( int i = 0; i < 16; i++){
		a[i] = new A;
	}

	A *ptr = new A;
	B *ptr1 = new B;

A* a1 = new A;

	A* a2 = new A;

	A* a3 = new A;

	A* a4 = new A;

	A* ptr = new A[100];

	delete [] ptr;

	for ( int i = 0 ; i < 16; i++)
		delete a[i];
	getch();
	return 0;
}
