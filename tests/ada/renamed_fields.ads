package Renamed_Fields is
    type Pair is record
        X : Integer;
        Text : String (1 .. 3);
    end record;
    R : Pair := (X => 3, Text => "abc");
    X : Integer renames R.X;
    Text : String renames R.Text;
end Renamed_Fields;
