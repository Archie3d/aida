with Ada.Text_IO; use Ada.Text_IO;
procedure EntryProducer is
    protected type Cell is
        entry Put (x : Integer);
        entry Get (x : out Integer);
    private
        full : Boolean := False;
        value : Integer := 0;
    end Cell;
    protected body Cell is
        entry Put (x : Integer) when not full is
        begin
            value := x;
            full := True;
        end Put;
        entry Get (x : out Integer) when full is
        begin
            x := value;
            full := False;
        end Get;
    end Cell;
    c : Cell;
    task Producer;
    task body Producer is
    begin
        for i in 1 .. 1000 loop
            c.Put (i);
        end loop;
    end Producer;
    x : Integer;
    sum : Integer := 0;
begin
    for i in 1 .. 1000 loop
        c.Get (x);
        sum := sum + x;
    end loop;
    Put_Line (Integer'Image (sum));
end EntryProducer;
