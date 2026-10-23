with Ada.Text_IO;
with Runtime_Array_Library; use Runtime_Array_Library;
procedure RuntimeArrayLibraryTasking is
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
        begin
            for J in 1 .. 40 loop
                declare
                    Copy : Text := Make;
                    Grid : Shape := M;
                begin
                    Inspect (Copy, Grid);
                    if Text'Last /= 3 or Copy /= A or Grid (3, 6) /= 9 then
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
    Exercise;
    if Results.Done /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("library arrays in task contexts ok");
end RuntimeArrayLibraryTasking;
