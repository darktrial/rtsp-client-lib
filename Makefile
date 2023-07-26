.PHONY : cosmo app clean
All:
	$(MAKE) -C live555/
	$(MAKE) -C app/

app:
	$(MAKE) -C app/

live555:
	$(MAKE) -C live555/
clean:
	$(MAKE) -C app/ clean
	$(MAKE) -C live555/ clean


