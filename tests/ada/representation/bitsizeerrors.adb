procedure BitSizeErrors is
    type Too_Small is (Zero, One, Two);
    for Too_Small'Size use 1;
    type Mapped is (Low, High);
    for Mapped'Size use 3;
    for Mapped use (Low => -5, High => 3);
    type Outside is (Low, High);
    for Outside use (Low => -5, High => 3);
    for Outside'Size use 3;
    type Wide is (A, B);
    for Wide'Size use 33;
    type Frozen is (A, B, C);
    Value : Frozen := A;
    for Frozen'Size use 3;
begin
    null;
end BitSizeErrors;
