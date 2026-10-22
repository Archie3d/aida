with Ada.Text_IO;
procedure SizeTasking is
    subtype Byte is Long_Integer range -128 .. 127;
    for Byte'Size use 8;
    subtype Wide is Integer range -128 .. 127;
    for Wide'Size use 64;
    Shared : Wide := -100;
    pragma Atomic (Shared);
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
            Local : Byte := -100;
            pragma Atomic (Local);
            procedure Advance (Item : in out Byte) is
            begin
                Item := Item + 1;
            end Advance;
        begin
            for I in 1 .. 20 loop
                Local := -100;
                Advance (Local);
                if Local /= -99 or Local + 5000000000 /= 4999999901 then
                    raise Program_Error;
                end if;
                Shared := -100;
                if Shared /= -100 then raise Program_Error; end if;
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
    if Results.Successes /= 4 or Results.Failures /= 0 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("sized scalar storage in concurrent tasks");
end SizeTasking;
