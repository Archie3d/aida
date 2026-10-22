with Ada.Text_IO;
procedure EnumTasking is
    type Code is (Low, Middle, High);
    for Code use (-128, 0, 127);
    pragma Atomic (Code);
    Shared : Code := Low;
    function Echo (Item : Code) return Code;
    pragma Import (C, Echo, "__test_enum_echo");
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
            Local : Code := Low;
            procedure Advance (Item : in out Code) is
            begin
                Item := Code'Succ (Item);
            end Advance;
        begin
            for I in 1 .. 20 loop
                Local := Low;
                Advance (Local);
                if Local /= Middle or Echo (Local) /= Middle then
                    raise Program_Error;
                end if;
                Local := Low;
                if Local /= Low or Echo (Local) /= Low then
                    raise Program_Error;
                end if;
                Shared := High;
                Shared := Low;
                if Shared not in Code then raise Program_Error; end if;
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
    Ada.Text_IO.Put_Line ("enumeration codes in atomic task storage");
end EnumTasking;
