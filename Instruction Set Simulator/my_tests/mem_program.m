# Sums the numbers 0 to 100.
# Fill memory with numbers from 0 to 100
#
8165    #0  liz $r1, 187       |	int stop(r1)=187;
8201    #1  liz $r2, 1         |	int step(r2)=1;
8000    #2  liz $r0, 0         |	for(int i(r0)=0; (r1-r0)==0;r0=r0+r2)
0B20    #3  sub $r3, $r1, $r0  |
BB08    #4  bz  $r3, 8         |	{
4800    #5  sw  $r0, $r0       |		mem[i]=i;
0008    #6  add $r0, $r0, $r2  |	}
C003    #7  j 3                |
# Sum all values               |
8700    #8  liz $r7, 0         |	int acc(r7)=0;
8165    #9  liz $r1, 187       |	int stop(r1)=187;
8000    #11 liz $r0, 0         |	for(int i(r0)=0;(r1-r0)==0;r0=r0+r2)
8201    #10 liz $r2, 1         |	int step(r2)=1;
0B20    #12 sub $r3, $r1, $r0  |
BB12    #13 bz  $r3, 18        |	{
4300    #14 lw  $r3, $r0       |		int num(r3)=mem[i];
07EC    #15 add $r7, $r7, $r3  |		r7=r7+r3;
0008    #16 add $r0, $r0, $r2  |	}
C00C    #17 j 12               |
# Display result               |
70E0    #18 put $r7            |	std::cout<<acc<<std::endl;
6800    #19 halt               |	return;
