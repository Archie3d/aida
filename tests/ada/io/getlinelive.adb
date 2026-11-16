with Ada.Text_IO;
with Ada.Text_IO.Text_Streams;
with Ada.Wide_Text_IO;
with Ada.Wide_Wide_Text_IO;
procedure GetLineLive is
    Text : String (1 .. 80);
    Small : String (1 .. 4);
    Wide : Wide_String (1 .. 80);
    Wide_Wide : Wide_Wide_String (1 .. 80);
    Last : Natural;
    Item : Character;
    End_Of_Line : Boolean;

    procedure Answer (Value : String) is
    begin
        Ada.Text_IO.Put_Line ("ANSWER " & Value);
        Ada.Text_IO.Flush;
    end Answer;
begin
    Ada.Text_IO.Put_Line ("PROGRAM READY");
    Ada.Text_IO.Flush;
    Ada.Text_IO.Get_Line (Text, Last);
    Answer (Text (1 .. Last));
    Ada.Text_IO.Get_Line (Text, Last);
    Answer (Text (1 .. Last));
    Answer (Ada.Text_IO.Get_Line);
    Ada.Wide_Text_IO.Get_Line (Wide, Last);
    if Last /= 4 or else Wide (1 .. Last) /= "wide" then
        raise Program_Error;
    end if;
    Answer ("wide");
    Ada.Wide_Wide_Text_IO.Get_Line (Wide_Wide, Last);
    if Last /= 5 or else Wide_Wide (1 .. Last) /= "wider" then
        raise Program_Error;
    end if;
    Answer ("wider");
    Ada.Text_IO.Skip_Line;
    Answer ("skipped");
    Ada.Text_IO.Get_Line (Small, Last);
    Answer (Small (1 .. Last));
    Ada.Text_IO.Get_Line (Small, Last);
    Answer (Small (1 .. Last));
    Ada.Text_IO.Get_Line (Small, Last);
    Answer (Small (1 .. Last));
    Ada.Text_IO.Get_Line (Small, Last);
    Answer (Small (1 .. Last));
    Ada.Text_IO.Get_Immediate (Item);
    if Item /= 'x' then raise Program_Error; end if;
    Ada.Text_IO.Look_Ahead (Item, End_Of_Line);
    if not End_Of_Line then raise Program_Error; end if;
    Ada.Text_IO.Skip_Line;
    Answer ("immediate");
    Character'Read (Ada.Text_IO.Text_Streams.Stream (Ada.Text_IO.Current_Input), Item);
    if Item /= 'x' then raise Program_Error; end if;
    Ada.Text_IO.Look_Ahead (Item, End_Of_Line);
    if not End_Of_Line then raise Program_Error; end if;
    Ada.Text_IO.Skip_Line;
    Answer ("stream");
end GetLineLive;
