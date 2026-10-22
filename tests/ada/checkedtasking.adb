with Ada.Text_IO;
procedure CheckedTasking is
    protected Results is
        procedure Overflow (Value : Integer);
        procedure Passed;
        procedure Failed;
        function Successes return Integer;
        function Failures return Integer;
    private
        Good : Integer := 0;
        Bad : Integer := 0;
        Target : Integer := 0;
    end Results;
    protected body Results is
        procedure Overflow (Value : Integer) is
        begin
            Target := Value * Value;
        end Overflow;
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        procedure Failed is
        begin
            Bad := Bad + 1;
        end Failed;
        function Successes return Integer is
        begin
            return Good;
        end Successes;
        function Failures return Integer is
        begin
            return Bad;
        end Failures;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            Low : Long_Integer := Long_Integer'First;
            Minus_One : Long_Integer := -1;
            Value : Long_Integer := 123;
            Caught : Boolean;
        begin
            for I in 1 .. 100 loop
                Caught := False;
                begin
                    Value := Low / Minus_One;
                exception
                    when Constraint_Error => Caught := True;
                end;
                if not Caught or else Value /= 123 then
                    raise Program_Error;
                end if;
                Caught := False;
                begin
                    Results.Overflow (Integer'Last);
                exception
                    when Constraint_Error => Caught := True;
                end;
                if not Caught or else Low rem Minus_One /= 0 then
                    raise Program_Error;
                end if;
            end loop;
            -- Re-enter the protected object after an arithmetic exception.
            Results.Passed;
        exception
            when others => Results.Failed;
        end Worker;
        type Worker_Group is array (1 .. 4) of Worker;
        Workers : Worker_Group;
    begin
        null;
    end Exercise;
begin
    Exercise;
    if Results.Successes /= 4 or Results.Failures /= 0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("checked arithmetic task contexts");
end CheckedTasking;
