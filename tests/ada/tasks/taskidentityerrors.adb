with Ada.Task_Identification;
procedure TaskIdentityErrors is
    use Ada.Task_Identification;
    task type Worker;
    task body Worker is
    begin
        null;
    end Worker;
    Object : Worker;
    Id : Task_Id := null;
    Leaked : Integer := Id.all;
    Type_Id : Task_Id := Worker'Identity;
    Argument_Id : Task_Id := Object'Identity (1);
    Invalid : Boolean := Integer'Callable;
    Not_Entry : Task_Id := Object'Caller;
    package Hidden is
        type Secret is limited private;
        type Pointer is access Secret;
        Object : Pointer;
    private
        task type Secret;
    end Hidden;
    package body Hidden is
        task body Secret is
        begin
            null;
        end Secret;
    end Hidden;
    Private_Status : Boolean := Hidden.Object'Terminated;
    protected Guard is
        entry Wait;
        function Bad return Task_Id;
    end Guard;
    protected body Guard is
        entry Wait when True is
        begin
            null;
        end Wait;
        function Bad return Task_Id is
        begin
            return Wait'Caller;
        end Bad;
    end Guard;
begin
    Abort_Task (Id);
end TaskIdentityErrors;
