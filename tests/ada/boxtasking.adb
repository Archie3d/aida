with Ada.Text_IO;
with Box_Library;
procedure BoxTasking is
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Done : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Done := Done + 1;
        end Passed;
        function Count return Integer is
        begin
            return Done;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            Calls : Integer := 0;
            function Next return Integer is
            begin
                Calls := Calls + 1;
                return Calls;
            end Next;
            type Item is record
                Value : Integer := Next;
            end record;
            type Items is array (1 .. 3) of Item;
        begin
            for I in 1 .. 20 loop
                declare
                    A : Items := (others => <>);
                    B : Box_Library.Holder := Box_Library.Make;
                begin
                    if Calls /= I * 3 or B.Data(1).Value /= 42 or B.Text /= "xxx"
                        or A(1).Value + A(2).Value + A(3).Value /= 9 * I - 3 then
                        raise Program_Error;
                    end if;
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
    if Box_Library.A.Data(3).Value /= 42 or Box_Library.B(1).Pointer /= null then
        raise Program_Error;
    end if;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("box tasking and library defaults ok");
end BoxTasking;
