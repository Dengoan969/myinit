# myinit

result:
```
myinit demon started
line from config: '/usr/bin/bc -l /home/dengoan/myInit/in1.txt /home/dengoan/myInit/out1.txt
'task will start 0
line from config: '/usr/bin/sleep 5 /home/dengoan/myInit/in2.txt /home/dengoan/myInit/out2.txt
'task will start 1
line from config: '/usr/bin/bc -l /home/dengoan/myInit/in3.txt /home/dengoan/myInit/out3.txt
'task will start 2
Started task with index 0
Started task with index 1
Started task with index 2
task 2 finished with code: 0
Started task with index 2
task 2 finished with code: 0
Started task with index 2
task 2 finished with code: 0
Started task with index 2
task 2 finished with code: 0
Started task with index 2
task 2 finished with code: 0
Started task with index 2
myinit demon signal: 1
task 2 finished with code: 0
task 1 terminated by signal: 15
task 0 terminated by signal: 15
line from config: '/usr/bin/sleep 2 /home/dengoan/myInit/in4.txt /home/dengoan/myInit/out4.txt
'task will start 0
Started task with index 0
myinit demon signal: 2
task 0 finished with code: 0
myinit demon stopped
```
Процессы быстро отрабатывают (в зависимости от железа), поэтому можно не заметить по выводу в консоли что запущены все, но в логе всё пишется (task2 быстро работает и перезапускается несколько раз)
Вот как отрабатывало у меня:

![image](https://github.com/Dengoan969/myinit/assets/92169913/f82ee691-ac34-40bc-a79d-405be61f9411)
