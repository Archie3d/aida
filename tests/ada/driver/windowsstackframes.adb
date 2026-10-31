with Ada.Text_IO;
procedure WindowsStackFrames is
    protected Results is
        procedure Done;
        function Count return Natural;
    private
        Completed : Natural := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Completed := Completed + 1;
        end Done;
        function Count return Natural is
        begin
            return Completed;
        end Count;
    end Results;
    procedure Large_Frame is
        Data : String (1 .. 65536) := (others => 'x');
        procedure Touch (Item : in out String) is
        begin
            if Item'Length /= 65536 or Item (Item'First) /= 'x' or Item (Item'Last) /= 'x' then
                raise Program_Error;
            end if;
            Item (Item'Last) := 'y';
        end Touch;
    begin
        Touch (Data);
        if Data (Data'Last) /= 'y' then
            raise Program_Error;
        end if;
        Results.Done;
    end Large_Frame;
    procedure Exercise_Tasks is
        task type Worker;
        task body Worker is
        begin
            Large_Frame;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise_Tasks;
begin
    Large_Frame;
    Exercise_Tasks;
    if Results.Count /= 5 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("Windows stack frames passed");
end WindowsStackFrames;
