#!/usr/bin/env python

import os

class SigningRequirements:
	def able_to_sign():
		return False

class SigningRequirements_win(SigningRequirements):
	def __init__(self, cert_password, cert_filename):
		self.cert_password = cert_password
		self.cert_filename = cert_filename
	def able_to_sign(self):
		# code-signing cert may not be present -- shouldn't be for open source (unless user adds their own)
		if not os.path.exists(self.cert_filename):
			return False
		# password may have been left blank -- should be for open source (unless user adds their own)
		if not self.cert_password:
			return False
		return True

class SigningRequirements_mac(SigningRequirements):
	pass

class SigningRequirements_linux(SigningRequirements):
	pass