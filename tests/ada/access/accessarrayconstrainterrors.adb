procedure Accessarrayconstrainterrors is
    type Pointer is access String;
    subtype Pair is Pointer (1 .. 2);
    subtype Repeated is Pair (1 .. 2);
    subtype Wrong_Rank is Pointer (1 .. 2, 1 .. 2);
    subtype Named is Pointer (First => 1);
begin
    null;
end Accessarrayconstrainterrors;
