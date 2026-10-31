with Ada.Exceptions; use Ada.Exceptions;
package body Context_Worker is
    function Run (Mode : Integer) return Integer is
        procedure Meet;
        pragma Import (C, Meet, "contextMeet");
        Count : Integer := 0;
        procedure Fail is
        begin
            Count := Count + 1;
            Meet;
            raise Constraint_Error with "worker failure";
        end Fail;
        type Callback is access procedure;
        Action : Callback := Fail'Access;
        function Text return String is
        begin
            return "worker " & "failure";
        end Text;
        procedure Invoke is
        begin
            Action.all;
        exception
            when E : Constraint_Error =>
                if Exception_Message (E) /= Text then
                    raise Program_Error with "wrong nested message";
                end if;
                raise;
        end Invoke;
    begin
        begin
            Invoke;
        exception
            when E : Constraint_Error =>
                if Exception_Message (E) /= Text or Count /= 1 then
                    raise Program_Error with "wrong outer message";
                end if;
        end;
        if Mode = 1 then
            raise Program_Error with "uncaught worker";
        end if;
        return Count + 16;
    end Run;
end Context_Worker;
