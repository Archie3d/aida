with Ada.Text_IO; use Ada.Text_IO;
procedure TaskRendezvous is
    task type Server is
        entry Get (X : in out Integer; Text : out String);
        entry Stop;
    end Server;
    task body Server is
    begin
        accept Get (X : in out Integer; Text : out String) do
            X := X + 7;
            Text := "hello";
        end Get;
        accept Stop;
    end Server;
    S : Server;
    X : Integer := 3;
    Text : String (1 .. 5);
begin
    S.Get (X, Text);
    Put_Line (Integer'Image (X) & Text);
    S.Stop;
end TaskRendezvous;
