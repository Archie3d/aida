with Ada.Text_IO;
with Ada.Containers;
with Ada.Characters;
with Ada.Strings.Hash;
with Ada.Strings.Fixed.Hash;
with Ada.Strings.Bounded.Hash;
with Ada.Strings.Unbounded.Hash;
with Ada.Containers.Indefinite_Hashed_Maps;
procedure StringHashes is
    use type Ada.Containers.Hash_Type;
    package B is new Ada.Strings.Bounded.Generic_Bounded_Length (32);
    package U renames Ada.Strings.Unbounded;
    function Hash (Key : String) return Ada.Containers.Hash_Type renames Ada.Strings.Hash;
    function Bounded_Hash is new Ada.Strings.Bounded.Hash (Bounded => B);
    package Maps is new Ada.Containers.Indefinite_Hashed_Maps
        (String, Integer, Ada.Strings.Fixed.Hash, "=");
    procedure Check (Ok : Boolean) is
    begin
        if not Ok then
            raise Program_Error;
        end if;
    end Check;
    procedure Exercise is
        Key : String (Positive'Last - 2 .. Positive'Last) := "éÆé";
        Empty : String (7 .. 6);
        Dictionary : Maps.Map;
        Owned : U.Unbounded_String := U.To_Unbounded_String (Key);
        Bounded : B.Bounded_String := B.To_Bounded_String (Key);
    begin
        Check (Hash (Empty) = Hash (String'("")));
        Check (Hash (String'("")) = 2166136261);
        Check (Hash (String'("hello")) = 16#4F9F2CAB#);
        Check (Hash (Key) = Hash (String'("éÆé")));
        Check (Hash (Key) = Ada.Strings.Fixed.Hash (Key));
        Check (Hash (Key) = Bounded_Hash (Bounded));
        Check (Hash (Key) = Ada.Strings.Unbounded.Hash (Owned));
        Check (Hash (String'(1 => Character'Val (255))) /=
            Hash (String'(1 => Character'Val (0))));
        Check (Hash (String'(1 => Character'Last)) /= Hash (String'("a")));
        Maps.Insert (Dictionary, Key, 123);
        Check (Maps.Element (Dictionary, String'("éÆé")) = 123);
    end Exercise;
    task type Worker;
    task body Worker is
    begin
        Exercise;
    end Worker;
begin
    Exercise;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("string hashes passed");
end StringHashes;
