with Generic_Family;
with Child_Support;
package Child_Parent is new Generic_Family (Integer, 10, Child_Support.Increment);
