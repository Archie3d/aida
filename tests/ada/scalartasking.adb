with Ada.Text_IO;
with Scalar_Dynamic_Model; use Scalar_Dynamic_Model;
procedure ScalarTasking is
    protected Results is
        procedure Passed;
        function Done return Integer;
    private
        Count : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Count := Count + 1;
        end Passed;
        function Done return Integer is
        begin
            return Count;
        end Done;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            N : Integer := 3;
            subtype Local is Integer range 1 .. N;
            Edge : Long_Float := 2.5;
            subtype Local_Real is Long_Float range -Edge .. Edge;
            Value : Local_Real := 1.0;
        begin
            for I in 1 .. 40 loop
                if Captured.Bound'Last /= 12 or else Small'Width /= 3 or else Echo (12) /= 12 or else Local'Width /= 2
                    or else Real_Range'Last /= 4.5 or else Local_Real'Last /= 2.5 then
                    raise Program_Error;
                end if;
                N := 100;
                Edge := 100.0;
                begin
                    Value := 3.0;
                    raise Program_Error;
                exception
                    when Constraint_Error => null;
                end;
            end loop;
            Results.Passed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    Exercise;
    if Results.Done /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("dynamic scalar task contexts");
end ScalarTasking;
