int  counter=0;

int main(){

	for(int i=0;i<100;i++){

		 __sync_fetch_and_add(&counter,1);
	}
   return 0;
}
