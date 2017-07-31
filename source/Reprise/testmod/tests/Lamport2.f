program Main
  integer b(0:49, 0:49), a(0:49), x(0:49), k  
  integer i, j
  
  do i = 1, 9, 1
    do j = 1, 9, 1
      a(i+j) = b(i,j-1) + b(i-1,j)
      b(i,j) = a(i+j-2) + b(i,j)
    end do
  end do
end program Main