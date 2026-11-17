with Ada.Command_Line;
with Ada.Real_Time;
with Ada.Strings.Unbounded;
with Ada.Text_IO;

procedure FinalizationScaling is
    use Ada.Strings.Unbounded;
    Count : Natural := Natural'Value (Ada.Command_Line.Argument (1));
    type Integer_Access is access Integer;
    type Retained_Array is array (Positive range <>) of Integer_Access;
    Retained : Retained_Array (1 .. Count);
    Started : Ada.Real_Time.Time;
    Checksum : Natural := 0;
begin
    --  These allocations are unrelated to the timed controlled temporaries.
    for Index in Retained'Range loop
        Retained (Index) := new Integer'(Index);
    end loop;
    Started := Ada.Real_Time.Clock;
    for Iteration in 1 .. 2000 loop
        declare
            Text : Unbounded_String := To_Unbounded_String ("hello");
        begin
            Append (Text, '!');
            Checksum := Checksum + Length (Text);
        end;
    end loop;
    Ada.Text_IO.Put_Line ("Work seconds:" & Duration'Image (Ada.Real_Time.To_Duration
        (Ada.Real_Time."-" (Ada.Real_Time.Clock, Started))));
    Ada.Text_IO.Put_Line ("Checksum:" & Natural'Image (Checksum));
    if Count > 0 and then Retained (Count).all /= Count then raise Program_Error; end if;
end FinalizationScaling;
