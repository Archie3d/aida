with Ada.Text_IO;
with Float_Attribute_Checks;
procedure FloatAttributeTasking is
    procedure Check_Float is new Float_Attribute_Checks.Run (Float);
    procedure Check_Long is new Float_Attribute_Checks.Run (Long_Float);
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
                Check_Float;
                Check_Long;
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
    Ada.Text_IO.Put_Line ("floating attribute task contexts");
end FloatAttributeTasking;
