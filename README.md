1.Run g++ main.cpp -o compress_application.out <br>
2.then run ./compress_application.out <br>
3.Enter what you want to do compression or decompression <br>
4.Select the file on which you want to perform the task <br>
AND you can make the c++ code to an exe application by statically linknig the library code that will be used to run the application standalone <br>
use the command : g++ your_code.cpp -o my_app.exe -static -s -O3 <br>
this will create an exe application that will directly run by just double clicking on it <br> 
