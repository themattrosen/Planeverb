// draws magnitude frequency response of hamming window size 64

clear; //you have to clear variables before startng

x=0:0.01:50;  
in = 0:0.01:50;
inDB = 0:0.01:50;
for i = 1:5000
    in(i) = exp(-(i-40*100)/1000.0) * sin(i / 10);
    inDB(i) = 20.0 * log10(in(i));
end;

schroederOut = 0:0.01:50;
schroederOutDB = 0:0.01:50;
for i = 1:5001
    schroederOut(i) = 0.0;
    schroederOutDB(i) = 0.0;
end

next = 0.0;
for i = 5000:-1:1
    next = next + in(i) * in(i);
    schroederOut(i) = next;
    schroederOutDB(i) = 10.0 * log10(schroederOut(i));
end;

plot(x,in, x, schroederOutDB);

use = 5001*0.9;
xuse = x(1:use);
schroederUse = schroederOutDB(1:use);
[slope, intercept] = reglin(xuse,schroederUse);
endUse = schroederOutDB(use);
T60 = -60.0 / slope;
