with Ada.Containers.Doubly_Linked_Lists;
with Ada.Containers.Ordered_Maps;
with Ada.Containers.Ordered_Sets;
with Ada.Containers.Indefinite_Vectors;
procedure ContainerErrors is
    package Bad_List is new Ada.Containers.Doubly_Linked_Lists (String);
    package Bad_Map is new Ada.Containers.Ordered_Maps (Integer, String);
    type Limited_Item is limited record Value : Integer; end record;
    package Bad_Set is new Ada.Containers.Ordered_Sets (Limited_Item);
    package Texts is new Ada.Containers.Indefinite_Vectors (Positive, String);
    A : Texts.Vector;
    Stream : Integer := 0;
begin
    Texts.Insert (A, 1, Count => 2);
    Texts.Vector'Write (Stream, A);
end ContainerErrors;
