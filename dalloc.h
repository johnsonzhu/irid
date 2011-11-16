#ifndef _DEBUG_H
#define _DEBUG_H

/*
#define DMALLOC(p,t,size) \
do{\
	p = (t)malloc(size);\
	printf("%p malloc %d %s %d %s\n",p,size,__FILE__, __LINE__, __FUNCTION__);\
}while(0)

#define DCALLOC(p,t,num,size) \
do{\
	p = (t)calloc(num,size);\
	printf("%p calloc %d %d %s %d %s\n",p,num,size,__FILE__, __LINE__, __FUNCTION__);\
}while(0)

#define DREALLOC(p,t,size) \
do{\
	p = (t)realloc(p,size);\
	printf("%p realloc %d,%s,%d,%s\n",p,size,__FILE__, __LINE__, __FUNCTION__);\
}while(0)

#define DFREE(p) \
do{\
	printf("%p free %s,%d,%s\n",p,__FILE__, __LINE__, __FUNCTION__);\
	free(p);\
}while(0)
*/



#define DMALLOC(p,t,size) \
do{\
	p = (t)malloc(size);\
}while(0)

#define DCALLOC(p,t,num,size) \
do{\
	p = (t)calloc(num,size);\
}while(0)

#define DREALLOC(p,t,size) \
do{\
	p = (t)realloc(p,size);\
}while(0)

#define DFREE(p) \
do{\
	free(p);\
}while(0)


#endif
