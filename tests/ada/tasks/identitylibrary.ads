with Ada.Task_Identification;
package IdentityLibrary is
    Environment : constant Ada.Task_Identification.Task_Id := Ada.Task_Identification.Current_Task;
    task Worker;
    protected Results is
        procedure Publish (Id : Ada.Task_Identification.Task_Id);
        entry Await_Result (Id : out Ada.Task_Identification.Task_Id);
    private
        Ready : Boolean := False;
        Saved : Ada.Task_Identification.Task_Id;
    end Results;
end IdentityLibrary;
