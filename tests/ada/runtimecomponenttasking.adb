with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Runtime_Layout_Collision;
with Runtime_Layout_Model; use Runtime_Layout_Model;
procedure RuntimeComponentTasking is
    Limit : Integer := 3;
    type Slots is array (1 .. 4) of Pointer;
    protected Results is
        procedure Ticket (I : out Integer);
        procedure Save (I : Integer; P : Pointer);
        function Get (I : Integer) return Pointer;
    private
        Text : String (1 .. Limit) := (others => 'x');
        Count : Integer := 0;
        Values : Slots;
    end Results;
    protected body Results is
        procedure Ticket (I : out Integer) is
        begin
            Count := Count + 1;
            I := Count;
        end Ticket;
        procedure Save (I : Integer; P : Pointer) is
        begin
            Values (I) := P;
        end Save;
        function Get (I : Integer) return Pointer is
        begin
            if Text /= "xxx" then raise Program_Error; end if;
            return Values (I);
        end Get;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            Id : Integer;
        begin
            Results.Ticket (Id);
            for J in 1 .. 20 loop
                declare
                    Limit : Integer := Id + 1;
                    type Local is record
                        Data : String (1 .. Limit) := (others => 'x');
                        Last : Long_Integer := 17;
                    end record;
                    X : Local;
                    Copy : Holder := Make;
                    H : Hidden := Make_Hidden;
                begin
                    Check_Hidden (H);
                    Counted.Increment;
                    if X.Data'Length /= Limit or X.Last /= 17 or Copy.Data /= "ab"
                        or Heap.all'First /= 2 or Heap.all'Length /= 2 or Global.Last /= 99 or Table (2).First /= 11
                        or Three.Value.Data /= "bbb" or Three.Value.Last /= 3 then
                        raise Program_Error;
                    end if;
                end;
            end loop;
            Results.Save (Id, new String (Id .. Id + Id));
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
    procedure Free is new Ada.Unchecked_Deallocation (String, Pointer);
begin
    Runtime_Layout_Collision.Check;
    Exercise;
    if Counted.Value /= 80 then raise Program_Error; end if;
    for I in 1 .. 4 loop
        declare
            P : Pointer := Results.Get (I);
        begin
            if P = null or else P.all'First /= I or else P.all'Length /= I + 1 then
                raise Program_Error;
            end if;
            Free (P);
        end;
    end loop;
    Ada.Text_IO.Put_Line ("runtime component task contexts ok");
end RuntimeComponentTasking;
