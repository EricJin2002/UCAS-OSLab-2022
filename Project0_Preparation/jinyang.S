.global main
main:

// TODO: [p0-task1] Add your code here!

// the answer is stored in s1
    li s0,1
    li s1,0
    li s2,100

loop:
    // s1 = s0 + s1;
    add s1,s0,s1
    // s0++;
    addi s0,s0,1
    // if (s0 <= s2) continue;
    ble s0,s2,loop

end:
	j end