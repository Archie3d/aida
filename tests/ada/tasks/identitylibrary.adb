with Ada.Task_Identification;
package body IdentityLibrary is
    task body Worker is
    begin
        Results.Publish (Ada.Task_Identification.Current_Task);
    end Worker;
    protected body Results is
        procedure Publish (Id : Ada.Task_Identification.Task_Id) is
        begin
            Saved := Id;
            Ready := True;
        end Publish;
        entry Await_Result (Id : out Ada.Task_Identification.Task_Id) when Ready is
        begin
            Id := Saved;
        end Await_Result;
    end Results;
end IdentityLibrary;
