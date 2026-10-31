with Ada.Text_IO;
procedure FixedTasking is
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
                    type Fixed is delta 2.0 ** (-40) range -10.0 .. 10.0;
                    for Fixed'Small use 2.0 ** (-42);
                    A : Fixed := 1.5;
                    B : Fixed := 0.5;
                    Upper : Fixed := 2.0;
                    subtype Window is Fixed range 1.0 .. Upper;
                    X : Window := A;
                    Product : Long_Float := A * B;
                begin
                    if Product /= 0.75 or Window'Fore /= 2 or X not in Window then
                        raise Program_Error;
                    end if;
                    begin
                        X := Window (B) + 1.0;
                        raise Program_Error;
                    exception
                        when Constraint_Error => null;
                    end;
                    if X /= A then
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
    Ada.Text_IO.Put_Line ("fixed task contexts");
end FixedTasking;
