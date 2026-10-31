package Unchecked_Model is
    type Integer_Link is access all Integer;
    type Constant_Link is access constant Integer;
    type Vector is array (Integer range <>) of Integer;
    type Vector_Link is access all Vector;
    type Matrix is array (Positive range <>, Positive range <>) of Integer;
    type Matrix_Link is access all Matrix;
    type Mutable (Present : Boolean := True) is record
        Common : aliased Integer := 4;
        case Present is
            when True => Value : aliased Integer := 9;
            when False => null;
        end case;
    end record;
    type Mutable_Link is access all Mutable;
    type Component is record
        Value : aliased Integer := 5;
    end record;
    type Root is tagged record
        Value : Integer := 7;
    end record;
    function Read (Object : Root) return Integer;
    function Access_Read (Object : access Root) return Integer;
    type Child is new Root with record
        Extra : Integer := 10;
    end record;
    overriding function Access_Read (Object : access Child) return Integer;
    type Root_Link is access all Root'Class;
end Unchecked_Model;
