with Ada.Text_IO;
with Ada.Text_IO.Decimal_IO;
procedure DecimalTasking is
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
                    type Fixed is delta 0.01 digits 18;
                    package IO is new Ada.Text_IO.Decimal_IO (Fixed);
                    Text : String (1 .. 6);
                    Last : Positive;
                    Parsed : Fixed;
                    A : Fixed := 1.5;
                    B : Fixed := 0.5;
                    Upper : Fixed := 2.0;
                    subtype Window is Fixed range 1.0 .. Upper;
                    X : Window := A;
                    Product : Long_Float := A * B;
                    Rounded : Fixed := Fixed'Round (A * Fixed'(0.25));
                    Truncated : Fixed := A * Fixed'(0.25);
                begin
                    IO.Put (Text, Rounded);
                    IO.Get (Text, Parsed, Last);
                    if Parsed /= Rounded or Last /= Text'Last then raise Program_Error; end if;
                    if Rounded /= 0.38 or Truncated /= 0.37 or Product /= 0.75 or Window'Fore /= 2 or X not in Window then
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
    Ada.Text_IO.Put_Line ("decimal task contexts");
end DecimalTasking;
