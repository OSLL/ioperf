;; This set of functions is aimed to make easier work with call stacks using Emacs
;; NOTE: there are a lot of hardcoded stuff here. Get rid of it


(defun select-current-line-addr ()
  "Selects line address pointed by cursor"
  (interactive)
  (skip-chars-backward "/_A-Za-z0-9.:")
  (setq start (point))
  (skip-chars-forward "/_A-Za-z0-9.:")
  (buffer-substring start (point))
)

;;HARDCODED :(
(defun get-prefix(a)
  (cond ((string= "" a) "fs")
	((string= "li" a) "include/linux")
	((string= "k" a) "kernel")
	((string= "mm" a) "mm")
	((string= "ext3" a) "fs/ext3"))
)

(defun goto-def ()
  "Opens pointed file at a given line in other window"
  (interactive)
  ;; D'oh another hardcoding
  (setq base-path "/Users/raeb/Downloads/linux-3.7.10")
  (setq dest (select-current-line-addr))

  (if (= 1 (length (split-string dest "/"))) 
      (setq finfo dest
	    prfx (get-prefix ""))
      (setq finfo (cadr (split-string dest "/"))
	    prfx (get-prefix(car (split-string dest "/")))))

  (setq file-name (car (split-string finfo ":")))
  (setq line (cadr (split-string finfo ":")))

  (setq file-path (format "%s/%s/%s" base-path prfx file-name))

  (switch-to-buffer-other-window (find-file-noselect file-path))
  (goto-line (string-to-int line))
)

(global-set-key (kbd "C-c C-g") 'goto-def)
