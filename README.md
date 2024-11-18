# CharacterDevice2

với hàm test() để kiểm tra việc ghi và đọc với device file có đường dẫn /dev/my_dev_0:

output test.c:

![image](https://github.com/user-attachments/assets/6866324e-b071-4e92-9378-276fca568dc1)

output kern.log:

![image](https://github.com/user-attachments/assets/34bdd5ab-d92d-4c7e-b09c-fc45eaaeaf45)

với hàm test2() để kiểm tra việc đồng bộ hóa giữa 3 luồng khi thao tác với cùng một device file:

![image](https://github.com/user-attachments/assets/d17c5ca0-8782-499b-824c-1d91cd8a0e06)
