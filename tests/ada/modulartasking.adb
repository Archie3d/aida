with Ada.Text_IO;
procedure ModularTasking is
    protected Results is
        procedure Passed;
        procedure Failed;
        function Successes return Integer;
        function Failures return Integer;
    private
        Good : Integer := 0;
        Bad : Integer := 0;
    end Results;
    protected body Results is
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
        begin
            for I in 1 .. 20 loop
                declare
                    type Word is mod 2 ** 64;
                    X : Word := Word'Last;
                    Z : Word := 0;
                    Text : constant String := Word'Image (X);
                begin
                    if X * X /= 1 or Word'Mod (-1) /= X or Text /= " 18446744073709551615" then
                        raise Program_Error;
                    end if;
                    begin
                        X := X / Z;
                        raise Program_Error;
                    exception
                        when Constraint_Error => null;
                    end;
                    if X /= Word'Last then
                        raise Program_Error;
                    end if;
                end;
            end loop;
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
    Ada.Text_IO.Put_Line ("modular task contexts");
end ModularTasking;
