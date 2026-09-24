-- Character-based string support (RM A.4.1).
package Ada.Strings is
    Space : constant Character := ' ';
    Length_Error, Pattern_Error, Index_Error, Translation_Error : exception;
    type Alignment is (Left, Right, Center);
    type Truncation is (Left, Right, Error);
    type Membership is (Inside, Outside);
    type Direction is (Forward, Backward);
    type Trim_End is (Left, Right, Both);
end Ada.Strings;
