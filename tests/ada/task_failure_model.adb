with Ada.Text_IO;
package body Task_Failure_Model is
    overriding procedure Finalize (object : in out Guard) is
    begin
        Ada.Text_IO.Put_Line ("library cleanup passed");
    end Finalize;
    function Fail return Integer is
    begin
        raise Constraint_Error with "library task activation failed";
        return 0;
    end Fail;
    task body Worker is
        initial : Integer := Fail;
    begin
        raise Program_Error;
    end Worker;
end Task_Failure_Model;
